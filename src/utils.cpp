#include "utils.hpp"

using namespace std;


pair<long long, long long> parse_id(const string &id) {
    int dash = id.find('-');
    long long ms = stoll(id.substr(0, dash));
    long long seq = stoll(id.substr(dash + 1));
    return {ms, seq};
}

bool id_ge(const string &a, const string &b) {
    auto [ams, aseq] = parse_id(a);
    auto [bms, bseq] = parse_id(b);
    if (ams != bms) return ams > bms;
    return aseq >= bseq;
}

bool id_le(const string &a, const string &b) {
    auto [ams, aseq] = parse_id(a);
    auto [bms, bseq] = parse_id(b);
    if (ams != bms) return ams < bms;
    return aseq <= bseq;
}


bool id_gt(const string &a, const string &b) {
    auto [ams, aseq] = parse_id(a);
    auto [bms, bseq] = parse_id(b);
    if (ams != bms) return ams > bms;
    return aseq > bseq;
}