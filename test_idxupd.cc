/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

namespace {
const std::string FILE_PATH = "/shared/index.txt";
const std::string IDX_MARKER = "idx ";
}

static bool line_get_idx(const std::string &str,
                         const std::string &pat,
                         std::string &idx)
{
    size_t mid = str.rfind(pat, 0);
    if (mid == std::string::npos) {
        return false;
    }

    idx = str.substr(mid + pat.length());

    return true;
}

static size_t get_idx(const std::string &path, const std::string &marker)
{
    std::fstream file(path, std::ios::in);
    if (!file.is_open()) {
        std::cout << "Failed to open " << path << std::endl;
        return 0;
    }

    size_t idx = 0;
    std::string line;
    std::string idx_str;
    while (std::getline(file, line)) {
        bool found = line_get_idx(line, marker, idx_str);
        if (found) {
            try {
                idx = std::stol(idx_str);
                std::cout << line << " found " << idx_str << " (" << idx << ")" << std::endl;
                ++idx;
                break;
            }
            catch (std::exception &e) {
                std::cout << " skip '" << line << "' due to exception: " << e.what() << std::endl;
                idx = 0;
            }
        } else {
            std::cout << " skip '" << line << "'" << std::endl;
        }
    }

    file.close();

    return idx;
}

static void update(const std::string &path, const std::string &marker, int idx)
{
    std::fstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        std::cout << "Failed to open " << path << std::endl;
        return;
    }

    file << marker << std::to_string(idx) << std::endl;

    file.close();
}

int main()
{
    size_t idx = get_idx(FILE_PATH, IDX_MARKER);

    // attempt to open file again in case it was missing
    update(FILE_PATH, IDX_MARKER, idx);

    return EXIT_SUCCESS;
}
