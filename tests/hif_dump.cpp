//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <iostream>
#include <string>

#include "hif/hif_read.hpp"
#include "hif/hif_write.hpp"


using S = Hif_base::Statement;

int main() {

    Hif_base::Statement node_stmt = Hif_base::create_node();
    node_stmt.instance = "my_node";
    node_stmt.type = 2;
    node_stmt.add_input("a", "foo");
    node_stmt.add_input("b", 100);
    node_stmt.add_output("y", "bar");
    node_stmt.add_attr("color", "orange");
    node_stmt.add_attr("size", (int64_t)64);

    Hif_base::Statement assign_stmt = Hif_base::create_assign();
    assign_stmt.instance = "assignment_stmt";
    assign_stmt.add_output("x", 44);

    Hif_base::Statement attr_stmt = Hif_base::create_attr();
    attr_stmt.instance = "my_attr";
    attr_stmt.add_attr("color", "blue");
    attr_stmt.add_attr("alpha", (int64_t)12);
    attr_stmt.add(true, "is_present");

    Hif_base::Statement open_call_stmt = Hif_base::create_open_call();
    open_call_stmt.instance = "call_func";
    open_call_stmt.add_input("array", "args");
    open_call_stmt.add_input("index", 7);

    Hif_base::Statement closed_call_stmt = Hif_base::create_closed_call();
    closed_call_stmt.instance = "call_foo_end";
    closed_call_stmt.add_output("ret");

    Hif_base::Statement open_def_stmt = Hif_base::create_open_def();
    open_def_stmt.instance = "define_adder";
    open_def_stmt.add_input("lhs");
    open_def_stmt.add_input("rhs");
    open_def_stmt.add_output("sum");

    Hif_base::Statement closed_def_stmt = Hif_base::create_closed_def();
    closed_def_stmt.instance = "define_adder";

    Hif_base::Statement end_stmt = Hif_base::create_end();
    end_stmt.instance = "end_define_adder";

    Hif_base::Statement use_stmt = Hif_base::create_use();
    use_stmt.instance = "use_function";
    use_stmt.add_attr("target", "adder");

    node_stmt.dump();
    assign_stmt.dump();
    attr_stmt.dump();
    open_call_stmt.dump();
    closed_call_stmt.dump();
    open_def_stmt.dump();
    closed_def_stmt.dump();
    end_stmt.dump();
    use_stmt.dump();

    return 0;
}
