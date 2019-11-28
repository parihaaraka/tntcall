#include <iostream>
#include <ev.h>
#include <fstream>

#include "cpp2tnt/connection.h"
#include "cpp2tnt/proto.h"
#include "cpp2tnt/mp_reader.h"
#include "cpp2tnt/mp_writer.h"
#include "cpp2tnt/ev4cpp2tnt.h"

using namespace std;
static int retcode = 0;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << R"(
Usage:
    tntcall <tarantool_connection_string> <function_name> [ARG][, ARG]...
    tntcall <tarantool_connection_string> -e <script_to_eval> [ARG][, ARG]...
    tntcall <tarantool_connection_string> -f <file_to_eval> [ARG][, ARG]...
ARG
    -n|[-i ]<arg>

    Where
        -i  Pass <arg> as integer
        -n  Pass nil
)";
        return 1;
    }

    tnt::connection cn;
    cn.on_error([](string_view message, tnt::error code, uint32_t db_error)
    {
        cerr << message << endl
             << "tnt connector code " << static_cast<int>(code)
             << ", db code " << db_error << endl;
        retcode = 1;
        ev_break(EV_DEFAULT);
    });

    cn.on_opened([&]()
    {
        try
        {
            iproto_writer w(cn);
            string script;
            int ind = 3; // index of next argument to pass to tnt
            if (argc > 3)
            {
                char *arg = argv[2];
                if (arg[0] == '-' && (arg[1] == 'e' || arg[1] == 'f') && arg[2] == '\0')
                {
                    if (arg[1] == 'e')
                        script = argv[ind];
                    else
                    {
                        ifstream f(argv[ind]);
                        if (f)
                            script.append(istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
                        else
                            throw runtime_error(string("unable to read ") + arg);
                    }
                    ++ind;
                }
            }

            if (script.empty())
                w.begin_call(argv[2]);
            else
                w.begin_eval(script);

            w.begin_array(uint32_t(argc - ind));
            for (int i = ind; i < argc; ++i)
            {
                char *arg = argv[i];
                if (arg[0] == '-' && (arg[1] == 'i' || arg[1] == 'n') && arg[2] == '\0')
                {
                    if (arg[1] == 'i')
                        w << strtoll(argv[++i], nullptr, 0);
                    else
                        w << nullptr;
                }
                else
                {
                    w << arg;
                }
            }
            w.finalize_all();
            cn.flush();
        }
        catch (const exception &e)
        {
            cerr << e.what() << endl;
            retcode = 1;
            ev_break(EV_DEFAULT);
        }
    });

    cn.on_response([](wtf_buffer &buf)
    {
        mp_reader bunch{buf};
        mp_reader r = bunch.iproto_message();
        try
        {
            auto encoded_header = r.map();
            auto encoded_body = r.map();
            encoded_header[tnt::header_field::CODE] >> retcode;
            if (retcode)
            {
                retcode &= 0x7fff;
                cerr << encoded_body[tnt::response_field::ERROR].to_string() << endl << "code " << retcode << endl;
            }
            else
            {
                auto response = encoded_body[tnt::response_field::DATA];
                auto tmp = response;
                auto inner_array = response.array();
                if (inner_array.cardinality() == 1)
                    cout << inner_array.to_string();
                else if (inner_array.cardinality() > 1)
                    cout << tmp.to_string();
            }
        }
        catch(const exception &e)
        {
            retcode = 1;
            cerr << e.what() << "\nresponse:\n" << hex_dump(r.begin(), r.end()) << endl;
        }
        ev_break(EV_DEFAULT);
    });

    ev4cpp2tnt ev_wrapper;
    ev_wrapper.take_care(&cn);
    cn.set_connection_string(argv[1]);
    cn.open();
    ev_run(EV_DEFAULT, 0);
    cn.close();
    return retcode;
}
