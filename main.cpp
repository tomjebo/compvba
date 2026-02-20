#include <CLI/CLI.hpp>      // CLI11 single include
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

static std::vector<uint8_t> read_all_bytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Failed to open input file: " + path);
    f.seekg(0, std::ios::end);
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);

    std::vector<uint8_t> buf;
    if (size > 0) {
        buf.resize(static_cast<size_t>(size));
        if (!f.read(reinterpret_cast<char*>(buf.data()), size))
            throw std::runtime_error("Failed to read input file: " + path);
    }
    return buf;
}

static void write_all_bytes(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) throw std::runtime_error("Failed to open output file: " + path);
    if (!bytes.empty() && !f.write(reinterpret_cast<const char*>(bytes.data()),
                                   static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("Failed to write output file: " + path);
}

// TODO: wire these to your existing algorithms
static std::vector<uint8_t> compress_bytes(const std::vector<uint8_t>& decompressed);
static std::vector<uint8_t> decompress_bytes(const std::vector<uint8_t>& compressed);

int main(int argc, char** argv) {
    CLI::App app{"compvba: MS-OVBA VBA compression/decompression tool"};

    // Optional: nicer error formatting
    app.set_help_flag("-h,--help", "Print help");
    app.require_subcommand(1);  // exactly one of compress/decompress required

    std::string inPath, outPath;

    auto add_io_options = [&](CLI::App* cmd) {
        cmd->add_option("-i,--in", inPath, "Input file path (binary)")
            ->required()
            ->check(CLI::ExistingFile);
        cmd->add_option("-o,--out", outPath, "Output file path (binary)")
            ->required();
    };

    auto* c = app.add_subcommand("compress", "Compress decompressed bytes into MS-OVBA compressed bytes");
    add_io_options(c);

    auto* d = app.add_subcommand("decompress", "Decompress MS-OVBA compressed bytes into decompressed bytes");
    add_io_options(d);

    try {
        app.parse(argc, argv);

        auto input = read_all_bytes(inPath);
        std::vector<uint8_t> output;

        if (*c) {
            output = compress_bytes(input);
        } else if (*d) {
            output = decompress_bytes(input);
        } else {
            // require_subcommand(1) should prevent this
            throw std::runtime_error("No operation selected.");
        }

        write_all_bytes(outPath, output);
        return 0;
    } catch (const CLI::ParseError& e) {
        return app.exit(e); // prints help/usage appropriately
    } catch (const std::exception& ex) {
        // Validation failures from our code path
        std::cerr << "Error: " << ex.what() << "\n\n";
        std::cerr << app.help() << "\n";
        return 2;
    }
}
