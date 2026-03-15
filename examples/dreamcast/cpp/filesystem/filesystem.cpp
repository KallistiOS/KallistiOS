/* KallistiOS ##version##

    filesystem.cpp
    Copyright (C) 2026 Yevhen Lohachov

    This example serves two purposes: to demonstrate the basic usage 
    of the C++17 std::filesystem API, and to serve as a validation test for
    the toolchain, to ensure that it's functioning properly. 
*/

#include <filesystem>
#include <print>
#include <chrono>

namespace fs = std::filesystem;

namespace {

void print_entry(const fs::directory_entry& entry) {
    if (entry.is_directory()) {
        std::print("[dir]\t{}\n", entry.path().string());
    } else if (entry.is_regular_file()) {
        std::print("[file]\t{} - {} bytes\n", entry.path().string(), entry.file_size());
    } else if (entry.is_symlink()) {
        std::print("[symlink]\t{}\n", entry.path().string());
    } else {
        std::print("[other]\t{}\n", entry.path().string());
    }
}

void print_perm(fs::perms p)
{
    using fs::perms;
    auto show = [=](char op, perms perm)
    {
        std::print("{}", (perms::none == (perm & p) ? '-' : op));
    };
    show('r', perms::owner_read);
    show('w', perms::owner_write);
    show('x', perms::owner_exec);
    show('r', perms::group_read);
    show('w', perms::group_write);
    show('x', perms::group_exec);
    show('r', perms::others_read);
    show('w', perms::others_write);
    show('x', perms::others_exec);
    std::print("\n");
}

} // namespace

int main(int argc, char* argv[]) {
    std::print("*** std::filesystem test ***\n");

    const fs::path ro_dir = fs::path("/rd");
    // const fs::path ro_dir = fs::path("/pc");
    // const fs::path ro_dir = fs::path("/cd");

    const fs::path rw_dir = fs::path("/ram");

    if (!fs::exists(ro_dir)) {
        std::print("[ERR] Path does not exist: {}\n", ro_dir.string());
        return 1;
    }
    if (!fs::exists(rw_dir)) {
        std::print("[ERR] Path does not exist: {}\n", rw_dir.string());
        return 1;
    }

    if (!fs::is_directory(ro_dir)) {
        std::print("[ERR] Path is not a directory: {}\n", ro_dir.string());
        return 1;
    }
    if (!fs::is_directory(rw_dir)) {
        std::print("[ERR] Path is not a directory: {}\n", rw_dir.string());
        return 1;
    }

    std::error_code ec;

    std::print("\n*** Setting current path ***\n");
    fs::current_path(ro_dir, ec);
    if (ec) {
        std::print("[ERR] Failed to set current path: {}\n", ec.message());
    }
    std::print("Current path: {}\n", fs::current_path(ec).string());
    if (ec) {
        std::print("[ERR] Failed to get current path: {}\n", ec.message());
    }
    
    auto src_file = ro_dir / "data/file.txt";
    auto dst_file = rw_dir / "copy.txt";
    auto nonexistent_file = rw_dir / "nonexistent_file.txt";
    auto hard_link_file = rw_dir / "hard_link.txt";
    auto symlink_file = rw_dir / "symlink.txt";
    
    std::print("\n*** Copying file test ***\n");
    fs::copy_file(src_file, dst_file, fs::copy_options::update_existing, ec);
    if (ec) {
        std::print("[ERR] Failed to copy file: {}\n", ec.message());
    } else {
        std::print("Copied file from {} -> {}\n", src_file.string(), dst_file.string());
    }

    std::print("\n*** Testing file existence ***\n");
    if (!fs::exists(dst_file)) {
        std::print("[ERR] File does not exist: {}\n", dst_file.string());
    } else {
        std::print("File exists: {}\n", dst_file.string());
    }

    if (!fs::exists(nonexistent_file)) {
        std::print("File does not exist: {}\n", nonexistent_file.string());
    } else {
        std::print("[ERR] File exists: {}\n", nonexistent_file.string());
    }

    std::print("\n*** Last write time ***\n");
    auto ftime = fs::last_write_time(dst_file, ec);
    if (ec) {
        std::print("[ERR] Failed to get last write time: {}\n", ec.message());
    } else {
        std::print("Last write time: {}\n", ftime);
    }

    std::print("\n*** Removing file test ***\n");
    fs::remove(dst_file, ec);
    if (ec) {
        std::print("[ERR] Failed to remove file: {}\n", ec.message());
    } else {
        std::print("Removed file: {}\n", dst_file.string());
    }

    std::print("\n*** Permissions test ***\n");
    print_perm(fs::status(src_file).permissions());

    std::print("\n*** Directory entries ***\n");
    for (const auto& entry : fs::directory_iterator(ro_dir, fs::directory_options::skip_permission_denied)) {
        print_entry(entry);
    }

    std::print("\n*** Recursive directory entries ***\n");
    for (const auto& entry : fs::recursive_directory_iterator(ro_dir, fs::directory_options::skip_permission_denied)) {
        print_entry(entry);
    }

    std::print("\n*** Creating directory test ***\n");
    fs::create_directory(rw_dir, ec);
    if (ec) {
        std::print("[ERR] Failed to create directory: {}\n", ec.message());
    } else {
        std::print("Created directory: {}\n", rw_dir.string());
    }

    std::print("\n*** Hard link test ***\n");
    fs::create_hard_link(src_file, hard_link_file, ec);
    if (ec) {
        std::print("[ERR] Failed to create hard link: {}\n", ec.message());
    } else {
        std::print("Created hard link: {} -> {}\n", src_file.string(), hard_link_file.string());
    }

    int hl_count = fs::hard_link_count(hard_link_file, ec);
    if (ec) {
        std::print("[ERR] Failed to get hard link count: {}\n", ec.message());
    } else {
        std::print("Hard link count: {}\n", hl_count);
    }

    std::print("\n*** Symlink test ***\n");
    fs::create_symlink(src_file, symlink_file, ec);
    if (ec) {
        std::print("[ERR] Failed to create symlink: {}\n", ec.message());
    } else {
        std::print("Created symlink: {} -> {}\n", src_file.string(), symlink_file.string());
    }

    auto sym_target = fs::read_symlink(symlink_file, ec);
    if (ec) {
        std::print("[ERR] Failed to read symlink: {}\n", ec.message());
    } else {
        std::print("Symlink target: {}\n", sym_target.string());
    }
    
    std::print("\nDone.\n");
    return 0;
}
