# tntcall

### Usage

&nbsp;&nbsp;&nbsp;&nbsp;`tntcall <tarantool_connection_string> <function_name> [ARG][, ARG]...`

&nbsp;&nbsp;&nbsp;&nbsp;`tntcall <tarantool_connection_string> -e <script_to_eval> [ARG][, ARG]...`

&nbsp;&nbsp;&nbsp;&nbsp;`tntcall <tarantool_connection_string> -f <file_to_eval> [ARG][, ARG]...`

ARG

&nbsp;&nbsp;&nbsp;&nbsp;`-n|[-i ]<arg>`

&nbsp;&nbsp;&nbsp;&nbsp; where

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`-i`  Pass `<arg>` as integer

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`-n`  Pass `nil`

Outputs json-like formatted data (you may try to use [jq](https://github.com/stedolan/jq) to deal with it).

### Example

```
$ ./tntcall 'user:pass@127.0.0.1:3301' -e 'local a,b = ...; return a+b, a-b, {ok=true}' -i 3 -i 2
>[5, 1, {"ok": true}]

$ ./tntcall 'user:pass@127.0.0.1:3301' -e 'return table.concat({...})' hello \ word
>"hello word"

$ ./tntcall 'user:pass@127.0.0.1:3301' os.date '%A %B %d'
>"Friday November 29"

$ ./tntcall 'user:pass@127.0.0.1:3301' -e 'return ...' -i 123
>123
```
