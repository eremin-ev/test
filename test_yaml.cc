/*
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <iostream>
//#include <fstream>
#include <string>
//#include <vector>

#include <yaml-cpp/yaml.h>

static void dumpSequence(const YAML::Node &node)
{
    for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
        YAML::Node node1 = it->second;
        if (node1.IsSequence()) {
            dumpSequence(node1);
        } else {
            std::cout << "Node " << it->first << " is " << it->second << "\n";
        }
    }
}

int main()
{
	YAML::Node config = YAML::LoadFile("test_yaml.yaml");

    std::cout << "config Type" << config.Type() << " isSequence " << config.IsSequence() << std::endl;

    for (YAML::const_iterator it = config.begin(); it != config.end(); ++it) {
        std::cout << "Node isMap" << it->IsMap() << "\n";
        YAML::Node node1 = *it;
        //if (node1.IsSequence()) {
            //dumpSequence(node1);
        //} else {
            std::cout << "Name " << node1["name"] << "\n";
            std::cout << "Powers " << node1["powers"].IsSequence() << "\n";
        //}
    }




#if 0
	if (config["lastLogin"]) {
		std::cout << "Last logged in: " << config["lastLogin"].as<DateTime>() << "\n";
	}

	const std::string username = config["username"].as<std::string>();
	const std::string password = config["password"].as<std::string>();
	login(username, password);
	config["lastLogin"] = getCurrentDateTime();

	std::ofstream fout("config.yaml");
	fout << config;
#endif


    return 0;
}
