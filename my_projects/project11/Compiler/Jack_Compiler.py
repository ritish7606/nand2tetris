import sys
import os
from token_reader import TokenReader
from symbol_table import SymbolTable
from error_reporter import ErrorReporter
from vm_writer import VMWriter
from compilation_engine import CompilationEngine


def get_vm_output_path(token_file: str) -> str:
    """Generate corresponding .vm file path for a given tokenized XML file."""
    base_name = os.path.basename(token_file)
    name_without_suffix = base_name[:-8]  # remove '_myT.xml'
    directory = os.path.dirname(token_file)
    return os.path.join(directory, f"{name_without_suffix}.vm")


def compile_file(token_file: str) -> tuple[int, int]:
    """Compile a single tokenized XML file into a VM file."""
    reporter = ErrorReporter()
    reader = TokenReader(token_file, reporter)
    vm_output = get_vm_output_path(token_file)
    writer = VMWriter(vm_output)
    symbols = SymbolTable()

    engine = CompilationEngine(reader, writer, symbols, reporter, token_file)
    engine.compile_class()

    writer.save()
    reporter.show()
    print(f"[✓] Successfully generated: {vm_output}")

    return len(reporter.errors), len(reporter.warnings)


def collect_token_files(input_path: str) -> list[str]:
    """Find all tokenized XML files to compile."""
    files_to_process = []

    if os.path.isdir(input_path):
        for entry in os.listdir(input_path):
            if entry.endswith("_myT.xml"):
                files_to_process.append(os.path.join(input_path, entry))
    elif os.path.isfile(input_path):
        if input_path.endswith("_myT.xml"):
            files_to_process.append(input_path)
        else:
            print("Error: Input file must end with '_myT.xml'.")
            sys.exit(2)
    else:
        print("Error: Invalid path. Please provide a valid file or directory.")
        sys.exit(2)

    if not files_to_process:
        print("No valid tokenized XML files found (expected '*_myT.xml').")
        sys.exit(2)

    return files_to_process


def main():
    if len(sys.argv) < 2:
        print("Usage: python Compiler.py <source_file_or_directory>")
        sys.exit(2)

    input_path = sys.argv[1]
    token_files = collect_token_files(input_path)

    total_errors = total_warnings = 0

    for file_path in token_files:
        print(f"[INFO] Compiling: {file_path}")
        errors, warnings = compile_file(file_path)
        total_errors += errors
        total_warnings += warnings

    print("\n[SUMMARY]")
    print(f" Files processed: {len(token_files)}")
    print(f" Total errors: {total_errors}")
    print(f" Total warnings: {total_warnings}")


if __name__ == "__main__":
    main()
