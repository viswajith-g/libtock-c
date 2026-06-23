#!/usr/bin/env python3
"""
Size breakdown tool for a plain C/C++ ELF (e.g. a Tock userspace app).
Classifies symbols by name prefix — no build directory needed.

Usage:
    python3 elf_breakdown.py <path_to_elf>

Example:
    python3 elf_breakdown.py build/openthread_app.elf

Requires: arm-none-eabi-nm, c++filt
"""

import subprocess
import sys
import collections
import re


# ── Prefix-based classification ───────────────────────────────────────────────
#
# Rules are checked in order; first match wins.
# Add or reorder entries here to tune classification.

GROUPS = [
    # OpenThread C++ modules (demangled) — order matters, specific before general
    ("ot::Mle",               "ot::Mle::"),
    ("ot::Mac",               "ot::Mac::"),
    ("ot::Ip6",               "ot::Ip6::"),
    ("ot::Coap",              "ot::Coap::"),
    ("ot::Lowpan",            "ot::Lowpan::"),
    ("ot::MeshForwarder",     "ot::MeshForwarder::"),
    ("ot::MeshCoP",           "ot::MeshCoP::"),
    ("ot::NetworkData",       "ot::NetworkData::"),
    ("ot::NetworkDiagnostic", "ot::NetworkDiagnostic::"),
    ("ot::Crypto",            "ot::Crypto::"),
    ("ot::Tmf",               "ot::Tmf::"),
    ("ot::Utils",             "ot::Utils::"),
    ("ot::Instance",          "ot::Instance::"),
    ("ot::Message",           "ot::Message::"),
    ("ot::DataPollSender",    "ot::DataPollSender::"),
    ("ot::KeyManager",        "ot::KeyManager::"),
    ("ot::Tlv",               "ot::Tlv::"),
    ("ot::AnnounceSender",    "ot::AnnounceSenderBase::"),  # base first
    ("ot::AnnounceSender",    "ot::AnnounceSender::"),
    ("ot::EnergyScanServer",  "ot::EnergyScanServer::"),
    ("ot::Flash",             "ot::Flash::"),
    ("ot::SupervisionListener", "ot::SupervisionListener::"),
    ("ot::Settings",          "ot::Settings::"),
    ("ot::Notifier",          "ot::Notifier::"),
    ("ot::FrameBuilder",      "ot::FrameBuilder::"),
    ("ot::TimeTicker",        "ot::TimeTicker::"),
    ("ot::Timer",             ("ot::Timer::", "ot::TimerMilli")),
    ("ot::Neighbor",          ("ot::Neighbor::", "ot::NeighborTable::")),
    ("ot::Checksum",          "ot::Checksum::"),
    ("ot::BackboneRouter",    "ot::BackboneRouter::"),
    ("ot::MessageQueue",      ("ot::MessageQueue::", "ot::PriorityQueue::")),
    ("ot::ThreadNetif",       "ot::ThreadNetif::"),
    ("ot::LinkQuality",       ("ot::LinkQualityInfo::", "ot::RssAverager::", "ot::LqiAverager::")),
    ("ot::Random",            "ot::Random::"),
    ("ot::TrickleTimer",      "ot::TrickleTimer::"),
    ("ot::PanIdQueryServer",  "ot::PanIdQueryServer::"),
    ("ot::Tasklet",           ("ot::Tasklet::", "ot::TaskletIn")),
    ("ot::SecurityPolicy",    "ot::SecurityPolicy::"),
    ("ot::ThreadLinkInfo",    "ot::ThreadLinkInfo::"),
    ("ot::MessagePool",       "ot::MessagePool::"),
    ("ot::CslTxScheduler",    "ot::CslTxScheduler::"),
    ("ot::IndirectSender",    "ot::IndirectSender::"),
    ("ot::Ip4",               "ot::Ip4::"),
    ("ot::Heap",              "ot::Heap::"),
    ("ot::OffsetRange",       "ot::OffsetRange::"),
    ("ot::FrameData",         "ot::FrameData::"),
    ("ot::DataUtils",         "ot::DataUtils::"),
    ("ot::BinarySearch",      "ot::BinarySearch::"),
    ("ot::SuccessRateTracker","ot::SuccessRateTracker::"),
    ("ot::Radio",             "ot::Radio::"),
    ("ot::Parent",            "ot::Parent::"),
    ("ot::Router",            "ot::Router::"),
    ("ot::UriList",           "ot::UriList::"),
    # catch-all for any remaining ot:: symbols (globals, misc)
    ("ot (other)",            "ot::"),

    # OpenThread C API / platform
    ("openthread_platform",   ("otPlat", "platform")),
    ("openthread_api",        "ot"),          # otThreadSetEnabled etc.

    # Nordic radio driver
    ("nrf_802154",            "nrf_802154_"),
    ("nrfx",                  "nrfx_"),
    ("mpsl",                  "mpsl_"),

    # Crypto
    # AES lookup tables (FT0-3, RT0-3) are mbedtls static globals with no prefix
    ("mbedtls",               ("mbedtls_", "FT0", "FT1", "FT2", "FT3",
                               "RT0", "RT1", "RT2", "RT3", "RCON",
                               "aes_gen_tables")),

    # Zephyr net stack (if present)
    ("net_ipv6",              "net_ipv6"),
    ("net_if",                "net_if_"),
    ("net_context",           "net_context_"),
    ("net_conn",              "net_conn_"),
    ("net_buf",               "net_buf"),
    ("net_pkt",               "net_pkt_"),
    ("zsock",                 "zsock_"),

    # Zephyr kernel
    ("z_kernel",              ("z_", "k_", "_k_")),

    # libc / compiler runtime
    ("libc/printf",           ("_printf", "_svf", "_vf", "_sf", "_puts",
                               "_fflush", "_fwrite", "_read", "_write",
                               "_close", "_lseek", "_sbrk")),
    ("libc/runtime",          ("__aeabi_", "__udiv", "__div", "memcpy", "memset",
                               "memmove", "memcmp", "strlen", "strcpy", "strcmp",
                               "strncpy", "strncmp", "strcat", "strchr",
                               "__", "sqrt", "sin", "cos")),
]


def classify(sym: str) -> str:
    for label, prefixes in GROUPS:
        if isinstance(prefixes, str):
            if sym.startswith(prefixes):
                return label
        else:
            if any(sym.startswith(p) for p in prefixes):
                return label
    return "other"


# ── nm + c++filt ──────────────────────────────────────────────────────────────

def run_nm(elf_path: str) -> list[tuple[str, int, str]]:
    """Returns [(raw_symbol, size, kind), ...]"""
    try:
        out = subprocess.check_output(
            ["arm-none-eabi-nm", "--print-size", "--size-sort", elf_path],
            text=True, stderr=subprocess.DEVNULL,
        )
    except FileNotFoundError:
        sys.exit("Error: arm-none-eabi-nm not found.")
    except subprocess.CalledProcessError as e:
        sys.exit(f"nm failed: {e}")

    pairs = []
    for line in out.splitlines():
        parts = line.split()
        if len(parts) < 4:
            continue
        try:
            size = int(parts[1], 16)
        except ValueError:
            continue
        if size == 0:
            continue
        sym_type = parts[2]
        if sym_type not in "TtWwDdBbRr":
            continue
        kind = "text" if sym_type in "TtWw" else "data"
        pairs.append((parts[3], size, kind))
    return pairs


def demangle(raw_syms: list[str]) -> list[str]:
    if not raw_syms:
        return []
    try:
        result = subprocess.run(
            ["c++filt"],
            input="\n".join(raw_syms),
            capture_output=True, text=True, check=True,
        )
        return result.stdout.splitlines()
    except FileNotFoundError:
        sys.exit("Error: c++filt not found. Install binutils.")


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: elf_breakdown.py <path_to_elf>")

    elf = sys.argv[1]
    print(f"\nAnalyzing: {elf}")

    pairs = run_nm(elf)
    raw_syms = [p[0] for p in pairs]
    sizes    = [p[1] for p in pairs]
    kinds    = [p[2] for p in pairs]
    demangled = demangle(raw_syms)

    # group -> { text, data, symbols: [(sym, size, kind)] }
    groups = collections.defaultdict(lambda: {"text": 0, "data": 0, "symbols": []})

    for sym, size, kind in zip(demangled, sizes, kinds):
        label = classify(sym)
        groups[label][kind] += size
        groups[label]["symbols"].append((sym, size, kind))

    sorted_groups = sorted(
        groups.items(),
        key=lambda x: x[1]["text"] + x[1]["data"],
        reverse=True
    )

    grand_text = sum(d["text"] for d in groups.values())
    grand_data = sum(d["data"] for d in groups.values())

    print()
    print("=" * 80)
    print(f"  ELF BREAKDOWN")
    print(f"  text: {grand_text:,} B    data/bss: {grand_data:,} B    total: {grand_text+grand_data:,} B")
    print("=" * 80)
    print(f"  {'GROUP':<28} {'TEXT (B)':>10} {'DATA/BSS (B)':>13}")
    print(f"  {'-'*28}  {'-'*10}  {'-'*13}")

    for label, data in sorted_groups:
        t, d = data["text"], data["data"]
        if t + d < 32:
            continue
        print(f"  {label:<28} {t:>10,} {d:>13,}")
        top_syms = sorted(
            [(s, sz) for s, sz, k in data["symbols"] if k == "text"],
            key=lambda x: x[1], reverse=True
        )[:5]
        for sym, size in top_syms:
            short = sym[:76] + "…" if len(sym) > 76 else sym
            print(f"      {size:>8,}  {short}")

    print()
    print(f"  {'GRAND TOTAL':<28} {grand_text:>10,} {grand_data:>13,}")
    print()


if __name__ == "__main__":
    main()