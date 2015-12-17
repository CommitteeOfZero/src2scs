// SRC2SCS
// 5pb SRC to SCS converter
//
// Copyright Benjamin Moir 2015
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

bool islinefeed(int c)
{
	return c == '\n' || c == '\r';
}

std::string trim_extension(std::string str)
{
	return str.substr(0, str.find_last_of('.'));
}

int main(int argc, char **argv)
{
	std::string input, output;
	std::string tab = "\t";

	for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
		{
			printf("usage: src2scs -i input.src [parameters]\n\n");
			printf(" -h, --help\t\tPrints usage information and exits.\n");
			printf(" -v, --version\t\tPrints version and exits.\n");
			printf(" -i, --input file.src\tSpecifies input file.\n");
			printf(" -o, --output file.scs\tSpecifies output file.\n");
			printf(" -st, --space-tab\tUses four spaces instead of the tab character for indentation.\n");
			return 0;
		}
		else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
		{
			printf("src2scs v1.1\n");
			return 0;
		}
		else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--input"))
		{
			input = argv[i + 1];
			output = trim_extension(input) + ".scs";
		}
		else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
		{
			output = argv[i + 1];
		}
		else if (!strcmp(argv[i], "-st") || !strcmp(argv[i], "--space-tab"))
		{
			tab = "    ";
		}
	}

	// load script file
	std::ifstream ifs(input, std::ios::binary);
	std::ofstream ofs(output, std::ios::binary);
	std::string contents((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	std::istringstream ss(contents + '\n');
	ifs.close();
	
	// parse file
	bool label = false;
	while (ss)
	{
		char c = ss.get();
		while (ss && isspace(c))
			c = ss.get();

		switch (c)
		{
		case '/': // comments
			c = ss.get();
			if (c == '/') // Single line comment
			{
				if (label)
					ofs << tab.c_str();
				ofs << "//";
				while (ss)
				{
					c = ss.get();
					if (islinefeed(c))
					{
						while (ss && islinefeed(c))
							c = ss.get();
						ss.unget();
						break;
					}
					ofs << c;
				}
				ofs << '\n';
			}
			else if (c == '*') // Multi line comment
			{
				if (label)
					ofs << tab.c_str();
				ofs << "/*";
				while (ss)
				{
					c = ss.get();
					if (c == '*')
					{
						c = ss.get();
						if (c == '/')
							break;
					}
					ofs << c;
					if (islinefeed(c) && label)
						ofs << tab.c_str();
					if (islinefeed(c))
					{
						while (islinefeed(c))
							c = ss.get();
						ss.unget();
					}
				}
				ofs << "*/\n";
			}
			else
				ss.unget();
			break;

		case '#': // commands, labels
		{
			int argc = 0;
			std::string func;
			std::vector<std::string> argv;
			argv.push_back("");
			c = ss.get();

			// get command name
			while (ss && isspace(c) && !islinefeed(c))
				c = ss.get();
			while (ss && (isalnum(c) || c == '_'))
			{
				func.push_back(c);
				c = ss.get();
			}

			// get arguments (if any)
			while (ss && !islinefeed(c))
			{
				argv.push_back("");
				if (isspace(c) && !islinefeed(c))
				{
					while (ss && isspace(c) && !islinefeed(c))
						c = ss.get();
					ss.unget();
				}
				c = ss.get();
				while (ss && c != ',' && !islinefeed(c))
				{
					if (!islinefeed(c))
						argv[argc].push_back(c);
					c = ss.get();
				}
				argc++;
			}

			// indentation
			if (label && func.compare("label") && func.compare("include"))
				ofs << tab.c_str();

			// special cases
			if (!func.compare("label"))
			{
				argc = 0;
				func = argv[0] + ':';
				label = true;
			}
			else if (!func.compare("include"))
			{
				func = '#' + func;
			}
			else if (!func.compare("assign"))
			{
				argc = 0;
				func = argv[0] + " = " + argv[1] + ';';
			}
			else if (!func.compare("add"))
			{
				argc = 0;
				func = argv[0] + " += " + argv[1] + ';';
			}
			else if (!func.compare("sub"))
			{
				argc = 0;
				func = argv[0] + " -= " + argv[1] + ';';
			}
			else if (!func.compare("return") || !func.compare("end"))
			{
				label = false;
			}
			else if (!func.compare("mes"))
			{
				boost::replace_all(argv[0], "&", "\\");
				boost::replace_all(argv[0], "%", "\\");
				boost::replace_all(argv[0], "\"", "\\\"");
				argv[0] = '"' + argv[0] + '"';
			}
			else if (!func.compare("mes2v"))
			{
				boost::replace_all(argv[3], "&", "\\");
				boost::replace_all(argv[3], "%", "\\");
				boost::replace_all(argv[3], "\"", "\\\"");
				argv[3] = '"' + argv[3] + '"';
			}
			else if (!func.compare("call"))
			{
				if (!argv[0].compare("THIS"))
				{
					argc = 1;
					argv[0] = argv[1];
				}
				else
				{
					// I don't know how correct this is
					// the SCRBUF_ things are #defines from
					// the SCR headers. So SCRBUF_* isn't guaranteed to exist.
					func = "CallFar";
					argv[1] = '[' + argv[0] + ']' + argv[1];
					argv[0] = "SCRBUF_" + argv[0];
					boost::to_upper(argv[0]);
				}
			}

			// capitalize first character of func, if necessary
			if (func.compare("halt") && func.compare("jump") && func.compare("return")
				&& func.compare("end") && func.compare("wait") && func.compare("mwait"))
			{
				func[0] = toupper(func[0]);
			}

			// write to file
			ofs << func.c_str();
			if (argc > 0)
			{
				ofs << ' ';
				for (int i = 0; i < argc; i++)
				{
					if (i > 0)
						ofs << ", ";
					ofs << argv[i].c_str();
				}
			}
			ofs << '\n';
		}

		default: // other stuff (even, dd, dw, db, STRING)
			if (islinefeed(c))
				break;
			int argc = 0;
			std::string func;
			std::vector<std::string> argv;
			argv.push_back("");

			// get command name
			while (ss && isspace(c) && !islinefeed(c))
				c = ss.get();
			while (ss && (isalnum(c) || c == '_'))
			{
				func.push_back(c);
				c = ss.get();
			}

			// get arguments (if any)
			while (ss && !islinefeed(c))
			{
				argv.push_back("");
				if (isspace(c) && !islinefeed(c))
				{
					while (ss && isspace(c) && !islinefeed(c))
						c = ss.get();
					ss.unget();
				}
				c = ss.get();
				while (ss && !islinefeed(c) && c != ',')
				{
					if (isspace(c) && !islinefeed(c))
					{
						while (isspace(c) && !islinefeed(c))
							c = ss.get();
						ss.unget();
					}
					if (!islinefeed(c) && c != ',')
						argv[argc].push_back(c);
					c = ss.get();
				}
				argc++;
			}

			// write to file
			if (label && func.compare("even")) ofs << tab.c_str();
			if (!func.compare("even")) ofs << '\n';
			ofs << func.c_str();
			if (argc > 0)
			{
				ofs << ' ';
				for (int i = 0; i < argc; i++)
				{
					if (i > 0)
						ofs << ", ";
					ofs << argv[i].c_str();
				}
			}
			ofs << '\n';
			break;
		}
	}

	ofs.close();
	return 0;
}
