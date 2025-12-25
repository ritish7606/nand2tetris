from token_reader import Token as TRToken
from token_reader import TokenReader
from symbol_table import SymbolTable
from vm_writer import VMWriter
from error_reporter import ErrorReporter

class LabelGenerator:
    def __init__(self):
        self.count = 0

    def generate(self, prefix):
        label = f"{prefix}{self.count}"
        self.count += 1
        return label
    
class CompilationEngine:
    def __init__(self, token_reader: TokenReader, vm_writer: VMWriter, symbol_table: SymbolTable, error_reporter: ErrorReporter, file_name: str):
        self.tr = token_reader
        self.vm = vm_writer
        self.st = symbol_table
        self.er = error_reporter
        self.file_name = file_name
        self.class_name = None
        self.label_gen = LabelGenerator()

    def _peek(self):
        return self.tr.peek()
    
    def _advance(self, expected_tag = None, expected_value = None):
        token = self.tr.advance()
        if token is None:
            self.er.add_error(self.file_name, self.tr.current_index, f"Unexpected end of file. Expected {expected_value or expected_tag}.")
            return None
        if expected_tag and token.tag != expected_tag:
            self.er.add_error(self.file_name, token.index, f"Expected token tag {expected_tag}, but got {token.tag}.")
            return None
        if expected_value and token.value != expected_value:
            self.er.add_error(self.file_name, token.index, f"Expected token value '{expected_value}', but got '{token.value}'.")
            return None
        return token
    
    def compile_class(self):
        token = self._peek()
        if token is None:
            self.er.add_error(self.file_name, 0, "No tokens to compile.")
            return
        if token.tag != "keyword" or token.value != "class":
            self.er.add_error(self.file_name, token.index, f"Expected 'class' keyword at the beginning of class declaration but got '{token.value}'.")
        
        self._advance("keyword", "class")
        class_name_token = self._advance("identifier")
        if class_name_token is None:
            self.er.add_error(self.file_name, 0, "Class name expected after 'class' keyword.")
            self.class_name = "Unknown"
        else:
            self.class_name = class_name_token.value

        self._advance("symbol", "{")

        while True:
            token = self._peek()
            if token is None:
                self.er.add_error(self.file_name, 0, "Unexpected end of file in class body.")
                break
            if token.tag == "symbol" and token.value == "}":
                break
            elif token.tag == "keyword" and token.value in ("static", "field"):
                self.compile_class_var_dec()
            elif token.tag == "keyword" and token.value in ("constructor", "function", "method"):
                self.compile_subroutine()
            else:
                self.er.add_error(self.file_name, token.index, f"Unexpected token in class body: {token.value}")
                self._advance()

    def compile_class_var_dec(self):
        kind_token = self._advance("keyword")
        kind = kind_token.value if kind_token else "field"
        type_token = self._advance()

        if not type_token:
            self.er.add_error(self.file_name, 0, "Type expected in class variable declaration.")

        if type_token.tag not in ("keyword", "identifier"):
            self.er.add_error(self.file_name, type_token.index, f"Invalid type '{type_token.value}' in class variable declaration.")

        type_name = type_token.value if type_token else "int"
        name_token = self._advance("identifier")

        if name_token is None:
            self.er.add_error(self.file_name, 0, "Variable name expected in class variable declaration.")
        else:
            self.st.define(name_token.value, type_name, "field" if kind == "field" else "static")

        while self._peek() and self._peek().tag == "symbol" and self._peek().value == ",":
            self._advance("symbol", ",")
            name = self._advance("identifier")
            if name is None:
                self.er.add_error(self.file_name, 0, "Variable name expected after ','.")
            else:
                self.st.define(name.value, type_name, "field" if kind == "field" else "static")
        self._advance("symbol", ";")

    def compile_subroutine(self):
        subr_token = self._advance("keyword")
        subr_type = subr_token.value if subr_token else "function"
        return_type_token = self._advance()
        if return_type_token is None:
            self.er.add_error(self.file_name, 0, "Return type expected in subroutine declaration.")
        return_type = return_type_token.value if return_type_token else "void"
        name_token = self._advance("identifier")
        if name_token is None:
            self.er.add_error(self.file_name, 0, "Subroutine name expected in subroutine declaration.")
            subr_name = "unknown"
        else:
            subr_name = name_token.value
        full_subr_name = f"{self.class_name}.{subr_name}"

        self.st.start_subroutine()
        if subr_type == "method":
            self.st.define("this", self.class_name, "arg")
        self._advance("symbol", "(")
        self.compile_parameter_list()
        self._advance("symbol", ")")

        self._advance("symbol", "{")

        while self._peek() and self._peek().tag == "keyword" and self._peek().value == "var":
            self.compile_var_dec()

        n_locals = self.st.var_count("var")
        self.vm.write_function(full_subr_name, n_locals)
        if subr_type == "constructor":
            n_fields = self.st.var_count("field")
            self.vm.write_push("constant", n_fields)
            self.vm.write_call("Memory.alloc", 1)
            self.vm.write_pop("pointer", 0)
        elif subr_type == "method":
            self.vm.write_push("argument", 0)
            self.vm.write_pop("pointer", 0)

        self.compile_statements()
        self._advance("symbol", "}")

    def compile_parameter_list(self):
        if self._peek() and self._peek().tag == "symbol" and self._peek().value == ")":
            return
        
        while True:
            type_token = self._advance()
            if type_token is None:
                self.er.add_error(self.file_name, 0, "Type expected in parameter list.")
            if type_token.tag not in ('keyword','identifier'):
                self.er.add_error(self.file_name, type_token.index, f"Invalid type '{type_token.value}' in parameter list.")
            name_token = self._advance("identifier")
            if name_token is None:
                self.er.add_error(self.file_name, 0, "Parameter name expected in parameter list.")
            self.st.define(name_token.value, type_token.value, "arg")
            if self._peek() and self._peek().tag == "symbol" and self._peek().value == ",":
                self._advance("symbol", ",")
            else:
                break
    
    def compile_var_dec(self):
        self._advance("keyword", "var")
        type_token = self._advance()
        type_name = type_token.value if type_token else "int"
        name = self._advance("identifier")
        if name is None:
            self.er.add_error(self.file_name, 0, "Variable name expected in variable declaration.")
        else:
            self.st.define(name.value, type_name, "var")
        while self._peek() and self._peek().tag == "symbol" and self._peek().value == ",":
            self._advance("symbol", ",")
            name = self._advance("identifier")
            if name is None:
                self.er.add_error(self.file_name, 0, "Variable name expected after ','.")
            else:
                self.st.define(name.value, type_name, "var")
        self._advance("symbol", ";")

    def compile_statements(self):
        while self._peek() and self._peek().tag == "keyword" and self._peek().value in ("let", "if", "while", "do", "return"):
            token = self._peek()
            if token.value == "let":
                self.compile_let()
            elif token.value == "if":
                self.compile_if()
            elif token.value == "while":
                self.compile_while()
            elif token.value == "do":
                self.compile_do()
            elif token.value == "return":
                self.compile_return()

    def compile_do(self):
        self._advance("keyword", "do")
        self.compile_subroutine_call()
        self._advance("symbol", ";")
        self.vm.write_pop("temp", 0)

    def compile_let(self):
        self._advance("keyword", "let")
        var_name_token = self._advance("identifier")
        if var_name_token is None:
            self.er.add_error(self.file_name, 0, "Variable name expected in let statement.")
            return
        var_name = var_name_token.value
        is_array = False
        if self._peek() and self._peek().tag == "symbol" and self._peek().value == "[":
            is_array = True
            self._advance("symbol", "[")
            self.compile_expression()
            self._advance("symbol", "]")
            kind = self.st.kind_of(var_name)
            index = self.st.index_of(var_name)
            if kind is None or index is None:
                self.er.add_error(self.file_name, var_name_token.index, f"Undefined variable '{var_name}' in let statement.")
                seg = 'local'
                index = 0
            else:
                seg = self._kind_to_segment(kind)
            self.vm.write_push(seg, index)
            self.vm.write_arithmetic("add")
        self._advance("symbol", "=")
        self.compile_expression()
        self._advance("symbol", ";")
        if is_array:
            self.vm.write_pop("temp", 0)
            self.vm.write_pop("pointer", 1)
            self.vm.write_push("temp", 0)
            self.vm.write_pop("that", 0)
        else:
            kind = self.st.kind_of(var_name)
            index = self.st.index_of(var_name)
            if kind is None or index is None:
                self.er.add_error(self.file_name, var_name_token.index, f"Undefined variable '{var_name}' in let statement.")
                seg = 'local'
                index = 0
            else:
                seg = self._kind_to_segment(kind)
            self.vm.write_pop(seg, index)

    def compile_while(self):
        self._advance("keyword", "while")
        self._advance("symbol", "(")
        start_label = self.label_gen.generate("WHILE_EXP")
        end_label = self.label_gen.generate("WHILE_END")
        self.vm.write_label(start_label)
        self.compile_expression()
        self.vm.write_arithmetic("not")
        self.vm.write_if(end_label)
        self._advance("symbol", ")")
        self._advance("symbol", "{")
        self.compile_statements()
        self._advance("symbol", "}")
        self.vm.write_goto(start_label)
        self.vm.write_label(end_label)

    def compile_if(self):
        self._advance("keyword", "if")
        self._advance("symbol", "(")
        self.compile_expression()
        self._advance("symbol", ")")
        false_label = self.label_gen.generate("IF_FALSE_")
        end_label = self.label_gen.generate("IF_END_")
        self.vm.write_arithmetic("not")
        self.vm.write_if(false_label)
        self._advance("symbol", "{")
        self.compile_statements()
        self._advance("symbol", "}")
        self.vm.write_goto(end_label)
        self.vm.write_label(false_label)
        if self._peek() and self._peek().tag == "keyword" and self._peek().value == "else":
            self._advance("keyword", "else")
            self._advance("symbol", "{")
            self.compile_statements()
            self._advance("symbol", "}")
        self.vm.write_label(end_label)
    
    def compile_return(self):
        self._advance("keyword", "return")
        if self._peek() and self._peek().tag == "symbol" and self._peek().value == ";":
            self.vm.write_push("constant", 0)
        else:
            self.compile_expression()
        self._advance("symbol", ";")
        self.vm.write_return()

    def compile_expression(self):
        self.compile_term()
        while self._peek() and self._peek().tag == "symbol" and self._peek().value in ("+", "-", "*", "/", "&", "|", "<", ">", "="):
            op_token = self._advance("symbol")
            self.compile_term()
            self._write_arithmetic_op(op_token.value)
    
    def compile_term(self):
        token = self._peek()
        if token is None:
            self.er.add_error(self.file_name, self.tr.current_index, "Unexpected end of file in term.")
            return
        if token.tag == "integerConstant" or token.tag == "int":
            t = self._advance("integerConstant")
            try:
                value = int(t.value)
            except ValueError:
                self.er.add_error(self.file_name, t.index, f"Invalid integer constant: {t.value}")
                value = 0
            self.vm.write_push("constant", value)
        elif token.tag == "stringConstant" or token.tag == "string":
            t = self._advance("stringConstant")
            string_value = t.value
            #print(f"String literal: '{string_value}', length={len(string_value)}")
            self.vm.write_push("constant", len(string_value))
            self.vm.write_call("String.new", 1)
            for char in string_value:
                self.vm.write_push("constant", ord(char))
                self.vm.write_call("String.appendChar", 2)
        elif token.tag == "keyword" and token.value in ("true", "false", "null", "this"):
            t = self._advance("keyword")
            if t.value == "true":
                self.vm.write_push("constant", 0)
                self.vm.write_arithmetic("not")
            elif t.value in ("false", "null"):
                self.vm.write_push("constant", 0)
            elif t.value == "this":
                self.vm.write_push("pointer", 0)
        elif token.tag == "symbol" and token.value == "(":
            self._advance("symbol", "(")
            self.compile_expression()
            self._advance("symbol", ")")
        elif token.tag == "symbol" and token.value in ("-", "~"):
            unary_op = self._advance("symbol")
            self.compile_term()
            if unary_op.value == "-":
                self.vm.write_arithmetic("neg")
            elif unary_op.value == "~":
                self.vm.write_arithmetic("not")
        elif token.tag == "identifier":
            ident_token = self._advance("identifier")
            name = ident_token.value
            if self._peek() and self._peek().tag == "symbol" and self._peek().value == "[":
                self._advance("symbol", "[")
                self.compile_expression()
                self._advance("symbol", "]")
                kind = self.st.kind_of(name)
                index = self.st.index_of(name)
                if kind is None or index is None:
                    self.er.add_error(self.file_name, ident_token.index, f"Undefined variable '{name}' in array access.")
                    seg = 'local'
                    index = 0
                else:
                    seg = self._kind_to_segment(kind)
                self.vm.write_push(seg, index)
                self.vm.write_arithmetic("add")
                self.vm.write_pop("pointer", 1)
                self.vm.write_push("that", 0)
            elif self._peek() and self._peek().tag == "symbol" and self._peek().value in ("(", "."):
                self.compile_subroutine_call_starting_with(name)
            else:
                kind = self.st.kind_of(name)
                index = self.st.index_of(name)
                if kind is None or index is None:
                    self.er.add_error(self.file_name, ident_token.index, f"Undefined variable '{name}' in term.")
                    seg = 'constant'
                    index = 0
                else:
                    seg = self._kind_to_segment(kind)
                self.vm.write_push(seg, index)
        else:
            self.er.add_error(self.file_name, token.index, f"Unexpected token in term: {token.value}")
            self._advance()
    

    def compile_subroutine_call(self):
        ident = self._advance('identifier')
        if ident:
            self.compile_subroutine_call_starting_with(ident.value)
    
    def compile_subroutine_call_starting_with(self,first_ident):
        n_args = 0
        if self._peek() and self._peek().tag == 'symbol' and self._peek().value == '.':
            self._advance('symbol','.')
            second = self._advance('identifier')
            if second is None:
                self.er.add_error(self.file_name, self.tr.current_index, "Expected subroutine name after '.'.")
                return
            sub_name = second.value
            kind = self.st.kind_of(first_ident)
            if kind is not None:
                index = self.st.index_of(first_ident)
                seg = self._kind_to_segment(kind)
                if index is None:
                    self.er.add_error(self.file_name, self.tr.current_index, f"Undefined variable '{first_ident}' in subroutine call.")
                    index = 0
                self.vm.write_push(seg, index)
                n_args += 1
                class_name = self.st.type_of(first_ident)
                full_name = f"{class_name}.{sub_name}"
            else:
                full_name = f"{first_ident}.{sub_name}"
                n_args=0
        else:
            full_name = f"{self.class_name}.{first_ident}"
            self.vm.write_push("pointer", 0)
            n_args = 1

        self._advance('symbol', '(')
        n_args += self.compile_expression_list()
        self._advance('symbol', ')')

        self.vm.write_call(full_name, n_args)

    def compile_expression_list(self):
        if self._peek() and self._peek().tag =='symbol' and self._peek().value == ')':
            return 0
        count = 0
        while True:
            self.compile_expression()
            count +=1
            if self._peek() and self._peek().tag =='symbol' and self._peek().value == ',':
                self._advance('symbol',',')
                continue
            break
        return count
    
    def _write_arithmetic_op(self,op):
        if op == '+':
            self.vm.write_arithmetic('add')
        elif op == '-':
            self.vm.write_arithmetic('sub')
        elif op == '*':
            self.vm.write_call('Math.multiply',2)
        elif op == '/':
            self.vm.write_call('Math.divide',2)
        elif op == '&':
            self.vm.write_arithmetic('and')
        elif op == '|':
            self.vm.write_arithmetic('or')
        elif op == '<':
            self.vm.write_arithmetic('lt')
        elif op == '>':
            self.vm.write_arithmetic('gt')
        elif op == '=':
            self.vm.write_arithmetic('eq')
        else:
            self.er.add_error(self.file_name,self.tr.current_index,f"Unknown operator '{op}'")
    
    def _kind_to_segment(self,kind):
        if kind == 'static': return 'static'
        if kind == 'field': return 'this'
        if kind == 'arg': return 'argument'
        if kind == 'var': return 'local'
        return 'local'