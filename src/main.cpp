// SRC2SCS
// 5pb SRC to SCS converter

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
	// no args?
	if (argc <= 1)
	{
		printf("Usage: src2scs input.src [output.scs]\n");
		return 0;
	}

	// load script file
	std::string newfile = argc <= 2 ? trim_extension(argv[1]) + ".scs" : argv[2];
	std::ifstream ifs(argv[1], std::ios::binary);
	std::ofstream ofs(newfile, std::ios::binary);
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
					ofs << '\t';
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
					ofs << '\t';
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
						ofs << '\t';
					if (islinefeed(c))
					{
						while (islinefeed(c))
							c = ss.get();
						ss.unget();
					}
				}
				ofs << "*/";
				ofs << '\n';
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
			while (ss && isspace(c))
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
				if (isspace(c))
				{
					while (ss && isspace(c))
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
				ofs << '\t';

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

			// write to file
			ofs << func.c_str();
			if (argc > 0)
			{
				ofs << " ";
				for (int i = 0; i < argc; i++)
				{
					if (i > 0)
						ofs << ", ";
					ofs << argv[i].c_str();
				}
			}
			ofs << '\n';
		}

		default: // other stuff (even, dd, dw, db)
			if (islinefeed(c))
				break;
			if (label)
				ofs << '\t';
			while (ss && !islinefeed(c))
			{
				ofs << c;
				c = ss.get();
			}
			ofs << '\n';
			break;
		}
		ofs.flush();
	}

	ofs.close();
	return 0;
}
