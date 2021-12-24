/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>
#include <map>
#include <string>

namespace {

enum val_ids {
    VAL_ID_INT,
    VAL_ID_UNSIGNED,
    VAL_ID_DOUBLE,
    VAL_ID_STRING,
};

const std::map<std::string, enum val_ids> key_types = {
    { "int", VAL_ID_INT },
    { "unsigned", VAL_ID_UNSIGNED },
    { "double", VAL_ID_DOUBLE },
    { "string", VAL_ID_STRING },
};

}

void get_value2(const std::string &, int &value)
{
    value = 1;
}

void get_value2(const std::string &, unsigned &value)
{
    value = 1U;
}

void get_value2(const std::string &, double &value)
{
    value = 1.0;
}

void get_value2(const std::string &, std::string &value)
{
    value = "one";
}

template <typename T>
int get_value(const std::string &key, T &value)
{
    try {
        enum val_ids id = key_types.at(key);

        get_value2(key, value);

        std::cout << id << " " << value << std::endl;
    }
    catch (std::out_of_range &e) {
        std::cout << "Not found " << key << std::endl;

        return 1;
    }

    return 0;
}

int main(void)
{
    int value1;
    get_value("int", value1);
    std::cout << "value1 " << value1 << std::endl;

    unsigned value2;
    get_value("unsigned", value2);
    std::cout << "value2 " << value2 << std::endl;

    double value3;
    get_value("double", value3);
    std::cout << "value3 " << value3 << std::endl;

    std::string value4;
    get_value("string", value4);
    std::cout << "value4 " << value4 << std::endl;

    //float value5;
    //get_value("float", value5);
    //std::cout << "value5" << value5 << std::endl;

    return 0;
}
