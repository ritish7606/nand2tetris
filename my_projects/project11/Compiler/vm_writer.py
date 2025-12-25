class VMWriter:
    def __init__(self, output_path):
        self.output_path = output_path
        self.lines = []

    def write_push(self,segment,index):
        self.lines.append(f"push {segment} {index}")
    
    def write_pop(self,segment,index):
        self.lines.append(f"pop {segment} {index}")

    def write_arithmetic(self,command):
        self.lines.append(command)

    def write_label(self,label):
        self.lines.append(f"label {label}")

    def write_goto(self,label):
        self.lines.append(f"goto {label}")

    def write_if(self,label):
        self.lines.append(f"if-goto {label}")

    def write_call(self,name,n_args):
        self.lines.append(f"call {name} {n_args}")

    def write_function(self,name,n_locals):
        self.lines.append(f"function {name} {n_locals}")

    def write_return(self):
        self.lines.append("return")

    def write_comment(self,comment):
        self.lines.append(f"// {comment}")

    def save(self):
        with open(self.output_path,'w') as f:
            for line in self.lines:
                f.write(line + '\n')
        self.lines = []