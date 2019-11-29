# tntcall

### Usage

`    tntcall <connection_string> <function_name> [ARG|SEP][, ARG|SEP]...`<br/>
`    tntcall <connection_string> -e <script_to_eval> [ARG|SEP][, ARG|SEP]...`<br/>
`    tntcall <connection_string> -f <file_to_eval> [ARG|SEP][, ARG|SEP]...`<br/>

ARG

`    -n`         nil<br/>
`    [-i ]<arg>` integer(-i) or string &lt;arg&gt;<br/>

SEP

`    -A`         begin array<br/>
`    -a`         end array<br/>
`    -M`         begin map<br/>
`    -m`         end map<br/>

Outputs json-like formatted data by means of [msgpuck](https://github.com/tarantool/msgpuck)'s mp_snprint().
You may try to use [jq](https://tracker.debian.org/pkg/jq) to deal with
the output.

### Examples

```
$ ./tntcall 'user:pass@127.0.0.1:3301' -e 'local a,b = ...; return a+b, a-b, {ok=true}' -i 3 -i 2
>[5, 1, {"ok": true}]

$ ./tntcall 'user:pass@127.0.0.1:3301' -e 'return table.concat({...})' hello \ word
>"hello word"

$ ./tntcall 'user:pass@127.0.0.1:3301' os.date '%A %B %d'
>"Friday November 29"

$ ./tntcall 'user:pass@127.0.0.1:3301' -e 'return ...' -i 123
>123

$ ./tntcall 'user:pass@127.0.0.1:3301' box.cfg -M memtx_memory -i 2147483648 -m
>
$ ./tntcall 'user:pass@127.0.0.1:3301' -e 'return box.cfg.memtx_memory'
> 2147483648
```
