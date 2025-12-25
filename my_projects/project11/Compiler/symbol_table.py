from collections import namedtuple

Symbol = namedtuple('Symbol',['type','kind','index'])

class SymbolTable:
    def __init__(self):
        self.class_scope = {}
        self.subr_scope = {}
        self.counts = {'static':0,'field':0,'var':0,'arg':0}

    def start_subroutine(self):
        self.subr_scope = {}
        self.counts['var'] = 0
        self.counts['arg'] = 0
    
    def define(self,name,type,kind):
        if kind not in self.counts:
            raise ValueError(f"Invalid kind: {kind}")
        index = self.counts[kind]
        self.counts[kind] += 1
        symbol = Symbol(type,kind,index)
        if kind in ('static','field'):
            self.class_scope[name] = symbol
        else:
            self.subr_scope[name] = symbol
    
    def var_count(self,kind):
        if kind not in self.counts:
            raise ValueError(f"Invalid kind: {kind}")
        return self.counts.get(kind,0)
    
    def kind_of(self,name):
        if name in self.subr_scope:
            return self.subr_scope[name].kind
        if name in self.class_scope:
            return self.class_scope[name].kind
        return None
    
    def type_of(self,name):
        if name in self.subr_scope:
            return self.subr_scope[name].type
        if name in self.class_scope:
            return self.class_scope[name].type
        return None

    def index_of(self,name):
        if name in self.subr_scope:
            return self.subr_scope[name].index
        if name in self.class_scope:
            return self.class_scope[name].index
        return None