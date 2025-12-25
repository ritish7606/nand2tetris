class ErrorReporter:
    def __init__(self):
        self.errors = []
        self.warnings = []

    def add_error(self,file_name, token_number, message):
        self.errors.append((file_name, token_number, message))

    def add_warning(self, file_name, token_number, message):
        self.warnings.append((file_name, token_number, message))

    def has_issues(self):
        return bool(self.errors or self.warnings)
    
    def show(self):
        for file_name, token_number, message in self.errors:
            print(f"Error in {file_name} at token {token_number}: {message}")
        for file_name, token_number, message in self.warnings:
            print(f"Warning in {file_name} at token {token_number}: {message}")
        if not self.errors and not self.warnings:
            print("No errors or warnings found.")