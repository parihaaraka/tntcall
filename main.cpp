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
    tntcall <connection_string> <function_name> [ARG|SEP][, ARG|SEP]...
    tntcall <connection_string> -e <script_to_eval> [ARG|SEP][, ARG|SEP]...
    tntcall <connection_string> -f <file_to_eval> [ARG|SEP][, ARG|SEP]...
ARG
    -n          nil
    [-i ]<arg>  integer(-i) or string <arg>
SEP
    -A          begin array
    -a          end array
    -M          begin map
    -m          end map

Outputs json-like formatted data by means of msgpuck's mp_snprint().
You may try to use jq (https://tracker.debian.org/pkg/jq) to deal with
the output.

Examples:
    $ ./tntcall 'user:pass@127.0.0.1:3301' -e \
        'local a,b = ...; return a+b, a-b, {ok=true}' -i 3 -i 2
    > [5, 1, {"ok": true}]

    $ ./tntcall 'user:pass@127.0.0.1:3301' -e 'return table.concat({...})' \
        hello \ word
    > "hello word"

    $ ./tntcall 'user:pass@127.0.0.1:3301' os.date '%A %B %d'
    > "Friday November 29"

    $ ./tntcall 'user:pass@127.0.0.1:3301' -e 'return ...' -i 123
    > 123

    $ ./tntcall 'user:pass@127.0.0.1:3301' box.cfg \
        -M memtx_memory -i 2147483648 -m
    $ ./tntcall 'user:pass@127.0.0.1:3301' -e 'return box.cfg.memtx_memory'
    > 2147483648
)" << flush;
        return 1;
    }

    tnt::connection cn;
    cn.on_error([](string_view message, tnt::error code, uint32_t db_error)
    {
        cerr << message << endl
             << "tnt connector code " << static_cast<int>(code)
             << ", db code " << db_error << endl << flush;
        retcode = 1;
        ev_break(EV_DEFAULT);
    });

    cn.on_opened([&]()
    {
        try
        {
            iproto_writer w(cn);
            string script;
            uint32_t ind = 3; // index of next argument to pass to tnt
            if (argc > 3)
            {
                const char *arg = argv[2];
                const char  opt = arg[1];
                if (arg[0] == '-' && (opt == 'e' || opt == 'f') && !arg[2])
                {
                    if (opt == 'e')
                        script = argv[ind];
                    else
                    {
                        ifstream f(argv[ind]);
                        if (f)
                            script.append(istreambuf_iterator<char>(f),
                                          std::istreambuf_iterator<char>());
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

            const uint32_t max_cardinality = uint32_t(argc) - ind;
            w.begin_array(max_cardinality);
            for (auto i = ind; i < static_cast<uint32_t>(argc); ++i)
            {
                const char *arg = argv[i];
                const char *opt = strchr("inAaMm", arg[1]);
                if (arg[0] == '-' && opt && *opt && !arg[2])
                {
                    switch (*opt)
                    {
                    case 'i':
                        w << strtoll(argv[++i], nullptr, 0);
                        break;
                    case 'n':
                        w << nullptr;
                        break;
                    case 'M':
                        w.begin_map(max_cardinality / 2);
                        break;
                    case 'A':
                        w.begin_array(max_cardinality);
                        break;
                    case 'a':
                    case 'm':
                        w.finalize();
                    }
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
            cerr << e.what() << endl << flush;
            retcode = 1;
            ev_break(EV_DEFAULT);
        }
    });

    cn.on_response([](wtf_buffer &buf)
    {
        mp_reader bunch{buf};
        try
        {
            mp_reader r = bunch.iproto_message();
            auto encoded_header = r.read<mp_map_reader>();
            auto encoded_body = r.read<mp_map_reader>();
            encoded_header[tnt::header_field::CODE] >> retcode;
            if (retcode)
            {
                retcode &= 0x7fff;
                cerr << encoded_body[tnt::response_field::ERROR].to_string() << endl
                     << "code " << retcode << endl << flush;
            }
            else
            {
                auto response = encoded_body[tnt::response_field::DATA];
                auto tmp = response;
                auto inner_array = response.read<mp_array_reader>();
                if (inner_array.cardinality() == 1)
                    cout << inner_array.to_string();
                else if (inner_array.cardinality() > 1)
                    cout << tmp.to_string();
            }
        }
        catch(const exception &e)
        {
            retcode = 1;
            cerr << e.what() << "\nresponse:\n"
                 << hex_dump(bunch.begin(), bunch.end()) << endl << flush;
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
