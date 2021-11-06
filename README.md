# HIF: Hardware Interchange Format

## Goals

HIF is intended to be a file format to interchange circuits/netlists/designs across different hardware design tools/compilers.


The main goals:

* Generic to support multiple tools that may have different hardware internal representation
* Reasonably fast to load/save files
* Reasonably compact representation
* API to read/write HIF files
* Version and tool dependence information
* Avoid to build parser/iterator for each tool save/load of their internal formats
* Use the same format to support control flow (tree) and graph (netlist) representation
* Multiple files can be concatenated and the semantics do not change
* The HIF file includes the hardware representation and attributes

Non goals:

* Translate IRs across tools. The goal is to be able to load/save, not to
  translate across IRs. E.g: a FIRRTL 'add' has different semantics than a
Verilog 'add'. The former has +1 bits output, the later can drop precision.


## Store information


### Tool and Version

Each tool and even version within a tool has potential different semantics for
their internal representation. HIF has a "tool"


## Format

HIF has a very simple syntax that resembles a CSV. Where each line is
terminated with a new line and the first character decides between 5 class of
lines: 

* `+` marks an entry in topological order. The inputs have been previously defined. The output could be used before or after
* `-` marks an entry where the inputs anywhere (before or after)
* `{` marks the beginning of a new scope which can be a module or function
* `}` marks the end of the last started scope
* `=` marks an edge or net attributes


The `+`, `-`, and `{` share the same format:

```
+type:id:output_list:input_list
-type:id:output_list:input_list
{type:id:output_list:input_list
```

* `type` is the identifier for the type of cell/entry to perform. Examples
  could be cells or nodes like `add` and `firrtl.rop` but also constructs like
 `assign` or whatever types of nodes are used in the hardware tool internal
 representation.
* `id` is a unique for the given cell. The `id` must be unique given the local scope. Different
scopes can share the same `id`
* `output_list` uses a bundle syntax to list all the outputs
* `input_list` uses a bundle syntax to list all the inputs. Some of the inputs could be constant. These constants can be seen as parameters or attributes.


The bundle syntax is a comma separated list of values that have an optional
assignment with the following syntax `([id=]id)[,[id=]id]+`.


`id` is any sequence of characters that excludes a `,` or `:`. If the
identifier must include a comma or double colon, a escape character must be
used. The only allowed escape characters are `\,`, `\:`, and `\\`.


The ids can be net names, strings, or numerical constants. The numerical
constants are always in hexadecimal (E.g: `ff`, `1f`). The net names starts
with the not escaped `\` character. Strings start with a character different
that do not match the regex `[0-9a-z\\]`.  If the id must be a string with that
matches the regex a space is added.


Examples of entries with the meaning:

An `add` with instance name of `foo` that adds net `bar` to `1` and connects the output to 
net `out`.  Attributes or other values can be added like 512 (`100` in hex)
line of code, in the file named "a file starting with a".

```
+add:foo:\out:\bar,1,file= a file starting with a,loc=100
```

Calls to other modules/instances use the same syntax:

```
+my_submodule:instance_id:out_net=Z,out_net2=Y:A=inp_net1,B=inp_net2
```


Constructs that have scope like modules, functions, or even if/else case
statements use the `{` and `}` to mark scope. The syntax for the begin of scope
is the same as the `+`/`-` syntax. It is just that it also marks a begin of
scope.

The end of scope is must simpler, it is just a `}` following with the list of
attributes.


To indicate overall, edge, or net attributes the `=` entry used.  The first
line set the attribute `attr1` to net name `net`. The second line sets `attr2`
attribute to the edge from net `dst` and `src`. The last line sets the global
`attr3` that applies to any following lines in the current scope.

```
=:\net::attr1=some
=:\dst:\src:attr2=some
=:::attr3=some
```

The following Verilog example (submodule.v) when represented as a netlist, can be translated to:

```
   1
   2   module inner(input z, output a, input y, output h);
   3     assign a = y & z;
   4     assign h = !(y&z);
   5   endmodule
   6   
   7   module submodule (input a, input b, output c, output d);
   8     inner foo(.y(a),.z(b),.a(c),.h(d));
   9   endmodule
  10   // some comment, and another
```

```
{conf:::tool=some_verilog_tool,version= 3.some_string.1
=::: file=submodule.v
{module:inner:\a,\h:\z,\y,order=a;b;c;d,bits=1;1;1;1,loc=2
+and::\a=Z:A=\y,B=\z,loc=3
+and::\tmp=Z:A=\y,B=\z,loc=4
+not::\h:\tmp,loc=4
}loc=5
{module:submodule:\c,\d:\a,\b,loc=7,order=a:b:c:d,bits=1;1;1;1,loc=7
+inner:foo:\d=h,\y=a:z=\b,y=\b,loc=8
}loc=9
+comment:::col=0,txt= some comment\, and another,loc=10
}
```

The previous example could also use this syntax to represent the same circuit:

```
{conf:::tool=some_verilog_tool,version= 3.some_string.1
=::: file=submodule.v,loc=2
{module:inner:\a,\h:\z,\y,order=a;b;c;d
+and::\a=Z:A=\y,B=\z,loc=3
=:::loc=4
+and::\tmp=Z:A=\y,B=\z
+not::\h:\tmp
}loc=5
{module:submodule:\c,\d:\a,\b,loc=7
+inner:foo:\d=h,\y=a:z=\b,y=\b,loc=8
}loc=9
+comment:::col=0,txt= some comment\, and another,loc=10
}
```

```
}:input_list
=from:to:input_list
```
