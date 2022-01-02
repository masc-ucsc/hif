# HIF: Hardware Interchange Format

## Goals

HIF is intended to be a file format to interchange circuits/netlists/designs
across different hardware design tools/compilers. It consists of a "data
format" and an "encoding" to efficiently store the data format.


The main goals:

* Generic to support multiple tools that may have different Hardware Internal
  Representation (HIF)
* Reasonably fast to load/save files
* Reasonably compact representation
* API to read/write HIF files to avoid custom parser/iterator for each tool
* Version and tool dependence information
* Use the same format to support control flow (tree) and graph (netlist)
  representation

Non goals:

* Translate IRs across tools. The goal is to be able to load/save, not to
  translate across IRs. E.g: a FIRRTL 'add' has different semantics than a
  Verilog 'add'. Also, different IRs may have different bitwidth inference
  rules.

## Related Background

There are several hardware design compiler/tools, but this document explanation
uses 6 common tools to showcase different semantics and how to interact with
HIF:

* CIRCT MLIR (https://github.com/llvm/circt), or just MILR.
* CHISEL CHIRRTL/FIRRRTL (https://github.com/chipsalliance/firrtl,
  https://circt.llvm.org/docs/RationaleFIRRTL/), or just FIRRTL.
* LiveHD LNAST (https://masc-ucsc.github.io/docs), or just LNAST.
* LiveHD Lgraph (https://masc-ucsc.github.io), or just Lgraph.
* Verilog Slang AST (https://github.com/MikePopoloski/slang), or just slang.
* Verilog RTL netlist, or just RTL.


There are several ways to classify the different hardware Intermediate
Representations (IR):

* Netlist or Tree. A netlist is a graph-like structure with `nodes` and
  `edges`. A `node` can have several `pins` where `edges` connect. An `edge`
  connects a pair of `pins`. A tree structure is the most typical Abstract
  Syntax Tree (AST) where tree statements, also called nodes in HIF, have an
  order and structure.

* Scoping allows to restrict the visibility of variable definitions in a
  tree-like structure. There are many possible ways to manage scopes but the
  most common is when "previous writes to upper scopes are visible in lower
  scopes, and writes to lower scopes are not visible to upper scopes unless a
  previous write exists". Managing scopes without SSA is more difficult because
  it requires a stack, as a result there are 2 scopes supported in HIF.  Simple
  scopes that are typical scopes with SSA and custom scopes that it is anything
  else with scopes.


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
statements.


HIF goal is to be extensible per tool but with a common syntax. There are only
statements in HIF, each statement has a `class`. Each statement class has some
customizable fields per tool but overall a statement can have a `type`,
`instance`, `ios`, and `attributes`.


The `instance` is just an alphanumeric identifier.

The `ios` is an ordered list of identifiers with an modifier to indicate if the
io is an `input` or an `output`.

The `attributes` is a sequence of assignments that assign an string (lhs) to
another (rhs). The rhs is the attribute name, the lhs is the attribute value.


The statement `class` is just an enumerate with these possible values:

* `node` class is used to represent nodes in a graph or AST. Each node has a
  `type`, `instance`, `ios`, and `attributes`. The `type` is tool dependent and
  specified the node type. the outputs must be SSA. This means that same output
  can only be written once.

  + A `node` statement example could be A FIRRTL firrtl.add addition node with an
    output connecting to net `foo`, and two input connected to `bar1` and
    `bar2`. The node can have custom per tool like `loc` or `xxx`.
  ```
  node add instance_string
     (output foo, input bar1, input bar2)
    @(loc=3,xxx=some_string_field_attr)
  ```

* `assign` class connects inputs to outputs. The number of inputs and outputs
  should match. Unlike most statements, the output does not need to be SSA.
  This means that the same variable can be written many times by different
  assignments. If there are any attribute, the attribute is applied to each
  output. 

  + An `assign` statement example could be used to express a statement like `out
    = out + 1`
  ```
  node add
     (output tmp_ssa, input out, input 1)
  assign
     (output out, input tmp_ssa)
  ```

* `attr` class is used to assign attributes to an identifier. It could be seen
  like an assign but it has no `ios`. The `attributes` are assigned to the
  `identifier`.

  + An `attr` statment assigns attributes:
  ```
  attr attr_id
     @(loc=100, foo=bar)
  ```

* `begin_open_scope` class marks the begin of a scope that can access the upper
  scope. The tool can use `type` to indicate further functionality. Scopes can
  also have `ios` and `attributes`. If variables first time used inside the
  scope must 'live' after the scope is closed, they may be added to the `ios`
  output list. The scope outputs do not need to be SSA.

  + A `begin_open_scope` statment could be the the taken path of an if-statement. `if a==0 { b = c + 1 }` could be encoded as:

  ```
  node ==
    (output tmp, input a, input 0)
  begin_open_scope if_taken
    (input tmp)
  node +
    (output tmp2, input c, input 1)
  assign
    (output b, input tmp2)
  end
  ```

* `begin_close_scope` class is similar to `begin_open_scope` but marks the
  beging of a scope without access to upper scope. As such, all the inputs and
  outputs must be explicitly marked. The tool can use the `type` field to
  indicate scope. 


* `begin_open_function` class is similar to the `begin_open_scope` but the key
  difference is that it is not called. A typical use is to to declare a lambda
  or function that could capture current scope variables as inputs. To call the
  created function a `node` statement can be used.

  +A `begin_open_function` example could be a CIRCT 'firrtl.module' that access
  upper scope 'firrtl.circuit' definitions.
  ```
  begin_open_function firrtl.module foo_mod
    (input xx, output yy)
  ```

* `begin_close_function` class is like the `begin_open_function` but without
  upper scope access.

  +A `begin_close_function` example is a module or function declaration like a verilog `module foo(input a, output c, input x)`:
  ```
  begin_close_scope module_begin foo
    (input a, output c, input x)
  ```

* `end` class marks the end of a previously started scope, closure, or
  function.

* `use` class allows to set attributes to the reminding statements in the
  current and lower open scopes. This is used to avoid repeating the same
  attribute for each statement. 

  +A `use` class example is to indicate the file name that applies to the following statments:
  ```
  use
    @(file_name=foo.v)
  ```

The attributes and ios use a tuple syntax. A tuple is an ordered sequence of
elements that can be named. The EBNF syntax, not binary encoding, for HIF data
format grammar:

```
start ::= tool_version statement*

tool_version ::= 'use' '@(' tool=ID ',' version=XX ')

statement ::= class (ID ID?)? ios? attributes?

ios ::= '(' io_tuple? ')'

attributes ::= '@(' attr_tuple? ')'

class ::= 'node' | 'assign' | 'attr'
        | 'begin_open_scope' | 'begin_close_scope'
        | 'begin_open_function' | 'begin_close_function'
        | 'end' | 'use'

attr_tuple ::= attr_entry ( ',' attr_entry )*
attr_entry ::= ID '=' ID

io_tuple ::= io_entry ( ',' io_entry )*
io_entry ::= ('input'|'output') ((ID '=' ID) | ID)
```


Where ID is any sequence of characters to specify a netlist name, constant, or
identifier. In languages like Verilog an identifier can have any character, and
others use `.` to separate struct fields. In HIF an ID can be any sequence of
bytes where the end is know because the encoding includes the ID size.


The first statement should be a `use` to indicate the `tool` and `version`.
This is needed to distinguish semantics.


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

#### How do you handle flip in FIRRTL?

The FIRRTL bundle can be expanded to the `ios` and the flipped field
can have the opposite direction:

```
begin_scope_function
  (input io.input.bar, input io.input.foo, output io.input.flipped)
```

#### How do you handle multiple drivers tri-state logic?

The recommendation here is to create a solution similar to the partial update.
If the language supports multiple simultaneous drivers on a single net, a
solution is to have a node with "bus" type. The bus type can have many inputs
and a single output. The `bus` can have or not valid bits, this is up to the
tool semantics.


#### Small Verilog example?

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


There are two set files `num.id` and `num.st`. The `num` is a decimal value.
For each number the id file contains all the `ID` declarations used by the stmt
file. Both files have less than 1M (2^20) entries (IDs for id file, or
statements or stmt file).


### `ID` encoding

The `ID` can have any string and numerical constant. Different tools have
different classes of constants. Verilog supports 0,1,x,z in the encoding.
FIRRTL only has 0,1 and a concept of unknown for the whole number (not per
bit). VHDL has even more states per bit.


An inefficient implementation will use strings to represent each constant, net
name, or string with some in-band encoding to distinguish between them. The
problem is that this will be inefficient and tool dependent to distinguish
between 'ff' as 255 or 'ff' string or 'ff' as net or edge connecting
input/outputs.

The id file has a list of all the identifiers by the statement file with the
same number. Each `ID` entry consists of 3 fields: declare type, declare size,
and payload.

* `declare type`: is a 4 bit field (`sttt`).  `ttt` is the type of `ID` and the
  `s` bits to select between 1 or 3 bytes for the string size (8 or 24 bits
   declare size total). The `declare` constant type (`sttt`) can be:

    + string (`ttt=000`): A string sequence that allows any character.

    + base2 (`ttt=001`): A little endian number of (0,1) values in two's
      complement.

    + base3 (`ttt=010`): A sequence of 2 base2 numbers. The first encodes the 0/1
      sequence.  The 2nd encodes the Verilog `x`. E.g: the 'b0?10 is encoded as
      "0010" and "0100". If the 2nd number is zero, it means that it can be encoded
      as base2 without loss of information.

    + base4 (`ttt=011`): 3 sequences of base2 numbers. The first is 01, the 2nd is
      0?, the third is 0z.

    + custom (`ttt=100`): A per tool sequence of bits to represent a constant.

    + `ttt` from 5 to 7 are reserved.

* `declare size` is a 12 or 20 bits to indicate the size of the entry.

The `declare` statement binary encoding looks like `sttt_xxxx` when
small bit is set (`s=1`) and `sttt_xxxx_yyyy_yyyy_zzzz_zzzz` when
`s=0`. In both cases `s` bit is the bit 0.


There can be up to 1M IDs. Without any optimization, each ID reference would
need 20bits. In hardware, some names are more frequently used that others.
E.g: in BOOM FIRRTL the top 32 names represent over 1/3 of all the
names. To leverage this, there are two reference long and short reference.

* long reference: `xxxxxxxx_xxxxxxxx_xxxxxee0` (ee is bit 1 and 2) is an ID that points the ID file
  with 20bits. `ee` encodes the type of ID:
  + `ee=00` is a port id for an input or attribute
  + `ee=01` is a port id for an output
  + `ee=10` is a value to assign to port or net connected

* short reference: `xxxxxee1` is a software managed cache for the most frequent IDs

* no reference: `11111111` (255) is used to indicate no valid ID which can be used to
  indicate end of sequence or no instance ID.


### statement encoding


The statement follows a regular structure:


The first 4 bits selects the statement class (`cccc`):

* `node` (`0` or `0000`)
* `assign` (`1` or `0001`)
* `attr` (`2` or `0011`)
* `begin_open_scope` (`3` or `0100`)
* `begin_close_scope` (`4` or `0101`)
* `begin_open_function` (`5` or `0110`)
* `begin_close_function` (`6` or `0111`)
* `end` (`7` or `1000`)
* `use` (`8` or `1001`)
* `9` to `15` are reserved


The next 12 bits indicate the `type` for all the statement class.

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

The binary encoded for `0.id` and `0.st` is the following:

The contents for `0.id`:
```
1000           # sttt: small size and string declare
0010           # 4 characters in tool
't'
'o'
'o'
'l'

0000           # sttt: small size and string declare
0000_0000_0000_0001_0001  # 17 characters in "some_harcoded_url"
's'
'o'
'm'
...            # the rest of the string
'r'
'l'

1000           # sttt: small size and string declare
0111           # 7 characters in "version"
'v'
...
'n'

1000           # sttt: small size and string declare
0101           # 5 characters in "alpha"
'a'
...
'a'
```

The contents for `0.st`:
```
1001           # cccc: use statement
1111_11111111  # no type
11111111       # no IOs (end of IDs value)
00000_00_0     # input with ptr 3 to constant buffer (tool)
00001_10_0     # input with ptr 2 to constant buffer (some_harcoded_url)
00010_00_0     # input with ptr 1 to constant buffer (version)
00011_10_0     # input with ptr 0 to constant buffer (alpha)
11111111       # no more attributes
```

In the previous example, there are 16 bytes in control and 33 bytes to store
the strings. 



## Compilation

HIF has a dual cmake and a bazel build setup to make it easy to use as a library.

### optional abseil

If your system has abseil, the library can go faster using the flat_hash_map.
To enable use the -DUSE_ABSL_MAP=1




