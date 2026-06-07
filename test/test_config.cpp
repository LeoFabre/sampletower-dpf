#include "SampleConfig.hpp"
#include <cstdio>
#include <string>
#include <cstdlib>
#include <cmath>
#define EXPECT_NEAR(a, b, eps) do {                                          \
    const double _a = double(a), _b = double(b), _e = double(eps);           \
    if (std::abs(_a - _b) > _e) {                                            \
        std::fprintf(stderr, "FAIL %s:%d: %.9f != %.9f (eps=%.6g)\n",        \
            __FILE__, __LINE__, _a, _b, _e);                                 \
        std::exit(1);                                                        \
    }                                                                        \
} while (0)
#define EXPECT_TRUE(c) do {                                                  \
    if (!(c)) { std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); \
        std::exit(1); }                                                      \
} while (0)

static void writeFile(const std::string& p, const std::string& c){
    FILE* f = fopen(p.c_str(),"wb"); fwrite(c.data(),1,c.size(),f); fclose(f);
}

int main(){
    writeFile("/tmp/st_cfg.json",
        R"({"samples":[{"path":"/a.wav","loop":true},{"path":"/b.wav","loop":false}]})");
    auto v = st::loadSampleConfig("/tmp/st_cfg.json");
    EXPECT_TRUE(v.size() == 2);
    EXPECT_TRUE(v[0].path == "/a.wav");
    EXPECT_TRUE(v[0].loop == true);
    EXPECT_TRUE(v[1].path == "/b.wav");
    EXPECT_TRUE(v[1].loop == false);

    // Missing file -> empty, no throw.
    auto e = st::loadSampleConfig("/tmp/st_nope.json");
    EXPECT_TRUE(e.empty());

    // Malformed JSON -> empty, no throw.
    writeFile("/tmp/st_bad.json", "{not json");
    auto b = st::loadSampleConfig("/tmp/st_bad.json");
    EXPECT_TRUE(b.empty());

    // Missing 'loop' defaults to false; missing 'path' entry skipped.
    writeFile("/tmp/st_part.json", R"({"samples":[{"path":"/c.wav"},{"loop":true}]})");
    auto p = st::loadSampleConfig("/tmp/st_part.json");
    EXPECT_TRUE(p.size() == 1);
    EXPECT_TRUE(p[0].path == "/c.wav");
    EXPECT_TRUE(p[0].loop == false);

    std::printf("test_config OK\n");
    return 0;
}
