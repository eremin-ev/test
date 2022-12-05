/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

void test1(const std::string &in_path, const std::string &out_path)
{
    std::ifstream in_file(in_path);
    std::vector<int> v;

    int a;
    while (in_file >> a) {
        v.push_back(a);
    }

    std::sort(v.begin(), v.end());

    std::ofstream out_file(out_path);
	std::ostream_iterator<int> out_iter(out_file, "\n");
    std::copy(v.begin(), v.end(), out_iter);

	// Or use explicit code like
	//
    // for (const auto &e : v) {
    //    out_file << e << '\n';
    // }
}

void test2(const std::string &in_path, const std::string &out_path)
{
    std::ifstream in_file(in_path);
    std::vector<int> v;

	std::string line;
	while (std::getline(in_file, line)) {
		std::istringstream iss(line);
		int a;
		if (!(iss >> a)) {
            // syntax error
            break;
        }

        v.push_back(a);
	}

    std::sort(v.begin(), v.end());

    std::ofstream out_file(out_path);
    for (const auto &e : v) {
       out_file << e << '\n';
    }
}

int main()
{
    //test1("test_file_sort_in.txt", "test_file_sort_out.txt");
    test2("test_file_sort_in.txt", "test_file_sort_out.txt");

    return 0;
}
