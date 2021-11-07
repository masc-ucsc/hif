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
tool=https://github.com/llvm/circt/FIRRTL,version=0.1.2
tool=https://github.com/masc-ucsc/livehd/Lgraph,version=some.3.xxx
tool=https://github.com/masc-ucsc/livehd/LNAST,version=alpha
```

#### How do you handle bidirectional or `inout`?

Verilog has `inout`, CIRCT has `analog`. The way to handle it is to
have an `input` and an `output` with the same name.

#### How do you handle multiple drivers tri-state logic?

The recomendation here is to create a solution similar to the partial update.
If the language supports multiple simultaneous drivers on a single net, the
solution is to have a node like "bus" that has many input drivers. The `bus`
can have or not valid bits, this is up to the tool semantics.


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


