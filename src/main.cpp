#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string>

const char* script = "\
var binary = Android[process.argv[2]];\n\
var target = binary.target[process.argv[3]];\n\
\n\
var sources = null;\n\
if(typeof(target.srcs) == \"string\")\n\
{\n\
    sources = Android[target.srcs];\n\
}\n\
else\
{\n\
    sources = target.srcs;\n\
}\n\
\n\
if(target.srcs_append != null)\n\
{\n\
    if(typeof(target.srcs_append) == \"string\")\n\
    {\n\
        sources = sources.concat(Android[target.srcs_append]);\n\
    }\n\
    else\n\
    {\n\
        sources = sources.concat(target.srcs_append);\n\
    }\n\
}\n\
\n\
if(binary.srcs != null)\n\
{\n\
    if(typeof(binary.srcs) == \"string\")\n\
    {\n\
        sources = sources.concat(Android[binary.srcs]);\n\
    }\n\
    else\n\
    {\n\
        sources = sources.concat(binary.srcs);\n\
    }\n\
}\n\
\n\
var cfiles = new Array();\n\
var cppfiles = new Array();\n\
for(i = 0; i < sources.length; i++)\n\
{\n\
    if(sources[i].substr(sources[i].lastIndexOf('.')+1) == \"cpp\")\n\
    {\n\
        cppfiles.push(sources[i]);\n\
    }\n\
    if(sources[i].substr(sources[i].lastIndexOf('.')+1) == \"c\")\n\
    {\n\
        cfiles.push(sources[i]);\n\
    }\n\
}\n\
var content = \"C=\";\n\
for(i = 0; i < cfiles.length; i++)\n\
{\n\
    content+=cfiles[i]+\" \";\n\
}\n\
\n\
content+=\"\\n\\nCPP=\";\n\
for(i = 0; i < cppfiles.length; i++)\n\
{\n\
    content+=cppfiles[i]+\" \";\n\
}\n\
\n\
var cflags = \"\";\n\
for(i = 0; binary.cflags != null && i < binary.cflags.length; i++)\n\
{\n\
    cflags+=binary.cflags[i]+\" \";\n\
}\n\
\n\
console.log(content);\n\
console.log(\"\\n\"+binary.name+\".a: compile\");\n\
console.log(\"\tar rcs out/\"+process.argv[3]+\"/$@ out/\"+process.argv[3]+\"/*.o\");\n\
console.log(\"\\ncompile: $(C) $(CPP)\");\n\
console.log(\"\t-mkdir out\");\n\
console.log(\"\t-mkdir out/\"+process.argv[3]);\n\
if(cfiles.length > 0)\n\
{\n\
    console.log(\"\tclang -fPIC $(C) -c -Iinclude -I../include -include limits.h -include unistd.h -include sys/user.h -include signal.h \"+cflags);\n\
}\n\
if(cppfiles.length > 0)\n\
{\n\
    console.log(\"\tclang -std=c++1y -fPIC $(CPP) -c -Iinclude -I../include -include string.h\");\n\
}\n\
console.log(\"\tmv *.o out/\"+process.argv[3]);\n\
console.log(\"\\nclean:\\n\trm -rf out\");\n\
";

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        printf("usage: horizon <Android.bp file path>\n");
        return ENOENT;
    }

    FILE* file = fopen(argv[1], "r");
    if(file == NULL)
        return ENOENT;

    std::string buffer;
    buffer.append("var Android = {\n");

    ssize_t read;
    size_t len = 0;
    char* line = NULL;
    while((read = getline(&line, &len, file)) != -1)
    {
        std::string str(line);
        if(str.find("//") != -1)
        {
            continue;
        }

        if(!isspace(line[0]) && line[0] != ']' && line[0] != '}')
        {
            str = ("\""+str);
            if(str.find("=") != -1)
            {
                str.replace(str.find(" = "),3, "\": ");
            }
            else
            {
                str.replace(str.find(" "),1, "\": ");
            }
            buffer.append(str);
        }
        else if(str.find("srcs: ") != -1)
        {
            if((str.find("[") != -1) && (str.find("+") != -1))
            {
                buffer.append("            srcs:\"");
                buffer.append(str.substr(str.find(":")+2, (str.find("+")-str.find(":"))-3));
                buffer.append("\",\n");
                buffer.append("            srcs_append: ");
                buffer.append(str.substr(str.find("[")));
            }
            else if(str.find("[") != -1)
            {
                buffer.append(str);
            }
            else if(str.find("+") == -1)
            {
                str.insert(str.find(":")+2,"\"");
                str.insert(str.find(","),"\"");
                buffer.append(str);
            }
        }
        else if(str.find("cppflags: ") != -1)
        {
            if((str.find("[") != -1) && (str.find("+") != -1))
            {
                buffer.append("cppflags: ");
                buffer.append(str.substr(str.find("[")));
            }
            else if(str.find("[") != -1)
            {
                buffer.append(str);
            }
            else if(str.find("+") == -1)
            {
                str.insert(str.find(":")+2,"\"");
                str.insert(str.find(","),"\"");
                buffer.append(str);
            }
        }
        else if(line[0] == ']')
        {
            buffer.append("],\n");
        }
        else
        {
            buffer.append(line);
        }
    }

    int position = 0;
    while((position = buffer.find("]", position)) != -1)
    {
        int test = position;
        position++;
        while(isspace(buffer[--test]))
        {
        }
        if(buffer[test] == ',')
        {
            buffer.erase(test, 1);
        }
    }
    position = 0;
    while((position = buffer.find("}", position)) != -1)
    {
        int test = position;
        while(isspace(buffer[--test]))
        {
        }
        if(buffer[test] == ',')
        {
            buffer.erase(test, 1);
            position--;
        }
        if(buffer[position+1] != ',')
        {
            buffer.insert(position+1, ",");
            position++;
        }
        position++;
    }

    buffer.append("}\n");
    position = buffer.rfind("}", buffer.length()-1);
    int test = position;
    while(isspace(buffer[--test]))
    {
    }
    if(buffer[test] == ',')
    {
        buffer.erase(test, 1);
    }

    printf("%s", buffer.c_str());
    printf("%s", script);

//nodejs main.js cc_library android > test

    return 0;
}
