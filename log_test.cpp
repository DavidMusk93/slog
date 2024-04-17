#include "log.h"
#include "config.h"
#include "file.h"
#include "str.h"

int main(int argc, char* argv[]) {
    slog::SizeRotate::Builder builder;
    builder.set_size(slog::Bytes::of("1m").value());
    builder.set_name("test"s).set_base("/tmp"s).set_num_files(6).set_buf_size(1024);
    auto logger = slog::Logger<slog::SizeRotate>::of(builder.Build());

    const int n = argc < 2 ? 102400 : atoi(argv[1]);
    for (int i = 0; i < n; ++i) {
        logger->Log(slog::internal::format(
                        "#{} this is a test: abcd-efg-hi-jk-lmn-opq-rst-uvw-xyz\n"sv, i),
                    {});
    }
}
