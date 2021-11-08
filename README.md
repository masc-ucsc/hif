# HIF: Hardware Interchange Format

## Goals

HIF is intended to be a file format to interchange circuits/netlists/designs
across different hardware design tools/compilers. It consists of a "data
format" and an "encoding" to efficiently store the data format.


The main goals:

* Generic to support multiple tools that may have different hardware Internal Representation (IR)
* Reasonably fast to load/save files
* Reasonably compact representation
* API to read/write HIF files to avoid custom parser/iterator for each tool
* Version and tool dependence information
* Use the same format to support control flow (tree) and graph (netlist) representation
* The HIF data format includes the hardware representation and attributes
* When the HIF data format is in SSA mode and it does not have custom nesting scopes, tools should
be able to read it, but may not be able to transform due to unknown semantics.

Non goals:

* Translate IRs across tools. The goal is to be able to load/save, not to
  translate across IRs. E.g: a FIRRTL 'add' has different semantics than
  a Verilog 'add'. Also, different IRs may have different bitwidth inference
  rules.

## Related Background

There are several hardware design compiler/tools, but this document explanation uses 6
common tools to showcase different semantics and how to interact with HIF:

* CIRCT MLIR (https://github.com/llvm/circt), or just MILR.
* CHISEL CHIRRTL/FIRRRTL (https://github.com/chipsalliance/firrtl, https://circt.llvm.org/docs/RationaleFIRRTL/), or just FIRRTL.
* LiveHD LNAST (https://masc-ucsc.github.io/docs), or just LNAST.
* LiveHD Lgraph (https://masc-ucsc.github.io), or just Lgraph.
* Verilog Slang AST (https://github.com/MikePopoloski/slang), or just slang.
* Verilog RTL netlist, or just RTL.


There are several ways to classify the different hardware Intermediate Representations (IR):

* Netlist or Tree. A netlist is a graph-like structure with `nodes` and
  `edges`. A `node` can have several `pins` where `edges` connect. An `edge`
  connects a pair of `pins`. A tree structure is the most typical Abstract
  Syntax Tree (AST) where tree statements, also called nodes in HIF, have an
  order and structure.

* Static Single Assignment (SSA). An IR is in which each destination is written
  once and the value is written before use. The SSA hardware IR must also have
  a way to reference a forward write. This is needed to connect structures in
  a loop.

* Scoping allows to restrict the visibility of variable definitions in
  a tree-like structure. There are many possible ways to manage scopes but the
  most common is when "previous writes to upper scopes are visible in lower
  scopes, and writes to lower scopes are not visible to upper scopes unless
  a previous write exists". Managing scopes without SSA is more difficult
  because it requires a stack, as a result there are 2 scopes supported in HIF.
  Simple scopes that are typical scopes with SSA and custom scopes that it is
  anything else with scopes.


MLIR is an tree-like SSA representation with scopes. FIRRTL, slang, and LNAST
are also tree-like representations but there is no SSA. The three can have
custom scopes or not. In FIRRTL and LNAST there are lowering passes that either
remove scopes or transform to simple scopes with SSA as handled by HIF. Lgraph
and RTL are graph-like representations when traversed in topological order,
they can comply with the SSA definition of single write and writes before
reads. Netlist representations do not have scopes beyond block/module
definition.


The goal of HIF is to have a common data format and binary encoding to
represent all those representations. As such, other hardware IRs should also be
able to map.


When the HIF uses SSA and only simple scopes tools should be able to read other
tools data representation. The nodes or statements may be "unknown",
effectively being a "blackbox", but the tool should be able to map to its
internal data structure as "blackboxes". Since nearly all the hardware IRs
support blackboxes, this should not be a problem. Obviously if most of the
nodes are unknown, the hardware tool should not be able to apply
transformations or changes on those blackbox nodes, but a tool with HIF read
and write should preserve semantics.


## Data Format


HIF is a sequence of statements that show connectivity and nesting across the
statements. Each statement can have a `type`, `instance`, `io`, and
`attributes`.


There are some statements that each tool can specialize with custom types:


* `node`: A graph node or statement with multiple ios and attributes. The
  `node` statement must be SSA. The `type` is tool dependent and specifies
  operation to perform. The attributes are assigned to the node `instance`. E.g
  of node use has a `type` of "firrtl.add", "verilog.reduceop", or
  "module.instance".

* `forward`: A forward reference. The output must be SSA. This connects a still
  not defined io.input to a new defined io.output.

* `assign`: Connects one to one the io.inputs to the io.outputs. The number of inputs
  and outputs should match. This is used to indicate that the output of the
  assign is NOT SSA. If there are any attribute, the attribute is applied to each output.

* `attr`: Similar to assign, but it does not indicate that the SSA is broken. Attributes
  will be assigned, but no other write changing contents exist. It is possible to
  replace all the `attr` for `assign` but the tool will be more inneficient and may
  need to perform an SSA pass afterwards.

* `begin_open_scope`: Marks the begin of a scope that can access the upper scope. The
  tool can use `type` to indicate further functionality. E.g of use are if/else
  code blocks with and without SSA.

* `begin_close_scope`: Marks the beging of a scope without access to upper
  scope. All the inputs and outputs must be explicitly marked. The tool can use
  the `type` field to indicate scope. E.g: slang could use `module`,
  `function` as types, FIRRTL could use `circuit`.

* `begin_custom_scope`: Marks the beginning of scope with custom rules about
  upper scope access. Some tools may not support to read this scope format
  because the typical blackbox is not enough to encapsulate it.

* `begin_close_function`: Similar to a close scope but not called. The `type`
  and attributes can provide addition information like a slang module vs
  a slang function.

* `begin_open_function`: Similar to open scope but the sequence of statements are
  not called unless explicitly call with a `node` to the `instance` name. E.g of use
  are CIRCT `firrtl.module` that access upper scope definitions. Another use would be
  a LNAST closure that captures the upper scope variables.

* `begin_custom_function`: A function call with custom semantics about variable scope.

* `end`: Marks the end of a previously started scope, closure, or function.

* `use`: Allows to set attributes to the reminding statements in the current
  and lower open scopes. This is used to avoid repeating the same attribute for
  each statement. E.g: an attribute is to indicate the source file. This allows
  to write it once instead of setting source file attribute for each statement.


The attributes and ios use a tuple syntax. A tuple is an ordered sequence of elements
that can be named. The EBNF syntax, not binary encoding, for HIF data format grammar:

```
start ::= tool_version statement*

tool_version ::= 'use' '@(' tool=ID ',' version=XX ')

statement ::= class (ID ID?)? ios? attributes?

ios ::= '(' io_tuple? ')'

attributes ::= '@(' attr_tuple? ')'

class ::= 'attr' | 'node' | 'forward' | 'assign' 
        | 'begin_open_scope' | 'begin_close_scope' | 'begin_custom_scope'
        | 'begin_open_function' | 'begin_close_function' | 'begin_custom_function'
        |  'end' | 'use'

attr_tuple ::= attr_entry ( ',' attr_entry )*
attr_entry ::= ID '=' ID

io_tuple ::= io_entry ( ',' io_entry )*
io_entry ::= ('input'|'output') ((ID '=' ID) | ID)
```


Where ID is any sequence of characters to specify a netlist name, constant, or
identifier. In languages like Verilog an identifier can have any character, and
others use `.` to separate struct fields. In HIF an ID can be any sequence of
bytes where the end is know because the encoding includes the ID size.


The first statement should be a `use` to indicate the `tool` and `version`. This
is needed to distinguish semantics.


### Data Format FAQ

#### How do you support partial updates?

Typically partial updates change some bits in a wire/net. It is up to the tool
on how to represent in HIF but a logical functionality is to have a `node` with 
a custom `type` like "setbits" and inputs `din`, `val` and `mask`. The result
replaces from `din` the `mask` bits with the bits in `val`.

If the output is not SSA, the setbits should be followed by an assign.


```
node setbits
  (output tmp, input din=som_net, input val=0xF, input mask=0xF0)
assign
  (output some_net, input tmp)
```

#### How do you know the order of arguments?

Tuples are ordered. The declaration order is the order of the input/outputs.


#### How do you encode the tool and version?

The `tool` name should be a full URL should be used with
the potential dialect targetted. The version is any string
that the tool will use, but a semantic versioning is a logical string.

```
tool=https://github.com/llvm/circt/FIRRTL,version=0.1.2
tool=https://github.com/masc-ucsc/livehd/Lgraph,version=some.3.xxx
tool=https://github.com/masc-ucsc/livehd/LNAST,version=alpha
```

#### How do you handle bidirectional or `inout`?

Verilog has `inout`, CIRCT has `analog`. The way to handle it is to
have an `input` and an `output` with the same name.

#### How do you handle multiple drivers tri-state logic?

The recommendation here is to create a solution similar to the partial update.
If the language supports multiple simultaneous drivers on a single net, a
solution is to have a node with "bus" type. The bus type can have many inputs
and a single output. The `bus` can have or not valid bits, this is up to the
tool semantics.


#### Small verilog example?

It is very tool dependent, but this simple Verilog:

```
   1
   2   module inner(input z, output signed [1:0] a, input y, output h);
   3     assign a = y + z;
   4     assign h = !(y&z);
   5   endmodule
   6   
   7   module submodule (input a, input b, output signed [0:0] c, output d);
   8     inner foo(.y(a),.z(b),.a(c),.h(d));
   9   endmodule
  10   // some comment, and another
```

could be encoded as:

```
use 
  @(tool=some_harcoded_url,version=alpha)
use 
  @(file=submodule.v)

begin_close_function module inner 
   (input z, output a, input y, output h)
  @(loc=2)
  attr
     (input a, output a)
    @(bits_begin=1, bits_end=0, sign=true)
  node add
     (output a, input y, input z)
    @(loc=3)
  node and
     (output tmp, input y, input z)
    @(loc=4)
  node not
     (output h, input tmp)
    @(loc=4)
end 
  @(loc=5)

begin_close_function module submodule
   (input a, input b, output c, output d)
  @(loc=7)
  attr
     (input c, output c)
    @(signed=true)
  node call submodule
     (y=a,z=b,a=c,h=d)
    @(instance=foo, loc=8)
end 
  @(loc=9)

node comment
  @(txt=some comment\, and another,loc=10)
```


## Encoding

The data format sections explains the information to store. The encoding section
explains how to have an efficient binary encoding.


### `ID` encoding

The `ID` can have any sequence of characters or string, but it could also
encode numerical constants, or net names. Different tools have different
classes of constants. Verilog supports 0,1,x,z in the encoding. FIRRTL only
has 0,1 and a concept of unknown for the whole number (not per bit). VHDL
has even more states per bit.


An inefficient implementation will use strings to represent each constant, net
name, or string with some in-band encoding to distinguish between them. The
problem is that this will be inefficient and tool dependent to distinguish
between 'ff' as 255 or 'ff' string or 'ff' as net or edge connecting
input/outputs.


The solution is to support the common numeric constants, strings, and net names
while supporting to extend with non-standard constant encoding like VHDL. HIF
splits the large constants in a `declare` statement and the `ID` reference.

The `ID` reference avoid repeating the constant all over. There are circular
buffers with the most recently declared type ID, net name ID, and constant ID.
New IDs are inserted to the circular buffer on declaration. The size of the
circular buffer is specified at the top of file with a maximum of 12 bits for
statement `type`, and 20 bits for net names and 20 bits for constants. 


ID declaration looks like new statement class (next section). To reference the
ID, there are different offsets:

* `xxxxxxx00` is an ID with an inlined constant value. The 6 x bits allow for constants
  from -32 to 31 without the need to reference the circular buffer. When used in `io_entry`
  the constant implies an input.

* `xxxxxni01` is an ID that points uses `n` bit to decide which circular
  buffer to point (`1` for net or `0` for constant). The `i` bit indicates if
  the field is an input. The pointer to the circular buffer is just 5 bits
  (`xxxxx`).

* `xxxxxxxx_xxxxxni01` is similar but has 8 additional bits for circular buffer pointer (13 total).

* `xxxxxxxx_xxxxxxxx_xxxxxni10` is similar but has 8 additional bits for circular buffer pointer (21 total).

* `111111111` is used to indicate end of ID sequence.


### statement encoding


The statement follows a regular structure:


The first 4 bits selects the statement class:

* `node` (`0`)
* `forward` (`1`)
* `assign` (`2`)
* `begin_open_scope` (`3`)
* `begin_close_scope` (`4`)
* `begin_custom_scope` (`5`)
* `begin_open_function` (`6`)
* `begin_close_function` (`7`)
* `begin_custom_function` (`8`)
* `end` (`9`)
* `use` (`10`)
* `declare` (`11`)
* `12` to `15` are reserved


The next 12 bits indicate the `type` for all the statement class with the
exception of the `declare` which uses the upper 4 bits to select the constant
type (`ttt`) and the `s` bits to select between 1 or 2 additional bits for the
string size (12 or 20 bits declare size total).

After the `declare` size, the binary sequence is directly encoded, and a new statement starts afterwards.

The `declare` constant type (`tttt`) can be:

* net (`ttt=0000`): A string sequence that allows any character and used to connect between inputs and outputs.

* string (`ttt=0001`): A string sequence that allows any character.

* base2 (`ttt=0010`): A little endian number of (0,1) values in two's complement.

* base3 (`ttt=0011`): A sequence of 2 base2 numbers. The first encodes the 0/1 sequence.
  The 2nd encodes the verilog `x`. E.g: the 'b0?10 is encoded as "0010" and
  "0100". If the 2nd number is zero, it means that it can be encoded as base2
  without loss of information.

* base4 (`ttt=0100`): 3 sequences of base2 numbers. The first is 01, the 2nd is 0?, the
  third is 0z.

* custom (`ttt=0101`): A per tool sequence of bits to represent a constant.

* The rest are reserved for future use.


The `declare` statement binary encoding looks like `1011_sttt_aaaa_aaaa` when
`s=1` and `1011_sttt_aaaa_aaaa_bbbb_bbbb` when `s=0`.


For the other statements, the 12 bit `type` select a type from the type buffer.
If the type is all ones (`0xFFF`) no type is used.


After the type, there is an optional `ID` that it is class/type dependent. A 8
bit `255` indicates no ID used.

After starts the sequence of input/outputs. This is a sequence if `ID` followed
by an ID of 255 which indicates end of sequence. The `io_entry` has two
possible options a `ID=ID` or just `ID`. The later is encoded as inline
constant zero (`0x00`) followed by the ID. The reason is that the lhs can not
be a numeric constant of `0` as allowed by the `xxxxxxx00` encoding, but a
string or a net name. The following sequence of attributes follows the same
semantics.


#### Example


The entry sequence:

```
use
  @(tool=some_harcoded_url,version=alpha)
```

binary encoded as (the comments are to explain):

```
1011           # declare statement for "tool"
1001           # small size and string declare
00000010       # 4 characters in tool
't'
'o'
'o'
'l'

1011           # declare statment for "some_harcoded_url"
1001           # small size and string declare
00010001       # 17 characters in "some_harcoded_url"
's'
'o'
'm'
...            # the rest of the string
'r'
'l'

1011           # declare statment for "version"
1001           # small size and string declare
00000111       # 7 characters in "version"
'v'
...
'n'

1011           # declare statment for "alpha"
1001           # small size and string declare
00000101       # 5 characters in "alpha"
'a'
...
'a'

1010           # use statement
1111_11111111  # no type
11111111       # no IOs (end of IDs value)
00011_0_1_01   # input with ptr 3 to constant buffer (tool)
00010_0_1_01   # input with ptr 2 to constant buffer (some_harcoded_url)
00001_0_1_01   # input with ptr 1 to constant buffer (version)
00000_0_1_01   # input with ptr 0 to constant buffer (alpha)
11111111       # no more attributes
```

In the previous example, there are 16 bytes in control and 33 bytes to store the strings. 



