import xml.etree.ElementTree as ET

token_tags = {
    "keyword" : "keyword",
    "symbol"  : "symbol",
    "identifier" : "identifier",
    "integerConstant" : "int",
    "stringConstant" : "string"
}

class Token:
    def __init__(self, tag, value, index):
        self.tag = tag
        self.value = value
        self.index = index

    def __repr__(self):
        return f"Token(tag={self.tag}, value={self.value}, index={self.index})"

class TokenReader:
    def __init__(self,file_path,reporter):
        self.file_path = file_path
        self.reporter = reporter
        self.tokens = []
        self.current_index = 0
        self._load_tokens()

    def _load_tokens(self):
        try:
            tree = ET.parse(self.file_path)
            root = tree.getroot()
            if not list(root):
                self.reporter.add_error(self.file_path, 0, "Empty token file — no tokens found.")
                return
            if root.tag != "tokens":
                self.reporter.add_error(self.file_path, 0, "Root tag is not 'tokens'")
                return
            idx = 0
            for elem in root:
                tag = elem.tag
                text = elem.text or ""
                text = text[1:-1]
                if tag not in token_tags:
                    self.reporter.add_error(self.file_path, idx, f"Unknown token tag: {tag}")
                    continue
                self.tokens.append(Token(tag, text, idx))
                idx += 1
        except ET.ParseError as e:
            self.reporter.add_error(self.file_path, 0, f"XML parsing error: {str(e)}")
        except Exception as e:
            self.reporter.add_error(self.file_path, 0, f"Failed to load tokens: {str(e)}")
    
    def has_more_tokens(self):
        return self.current_index < len(self.tokens)
    
    def advance(self):
        if self.has_more_tokens():
            token = self.tokens[self.current_index]
            self.current_index += 1
            return token
        else:
            return None
        
    def peek(self):
        if self.has_more_tokens():
            return self.tokens[self.current_index]
        else:
            return None
    
    def rewind(self):
        if self.current_index > 0:
            self.current_index -= 1

    def current_token(self):
        return self.current_index
    
    def reset(self):
        self.current_index = 0