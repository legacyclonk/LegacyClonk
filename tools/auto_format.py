#!/usr/bin/env python3

import argparse
from itertools import chain
from pathlib import Path
import re

NEWLINE = '\n'

def main():
	ap = argparse.ArgumentParser(
		description='Auto format LegacyClonk source code')
	ap.add_argument('lc_dir', type=Path)
	ap.add_argument('-w', '--write', action='store_true',
		help='If omitted only a dry run is performed.')
	args = ap.parse_args()

	global lc_dir, dry_run
	lc_dir = args.lc_dir
	dry_run = not args.write
	auto_format()

def auto_format():
	if dry_run:
		print('This is a dry run!')

	format_cmake_files()
	format_cpp_files()
	format_tool_files()
	format_yml_files()

	if dry_run:
		print('Dry run finished.')

def format_cmake_files():
	format_filelists()
	format_files((lc_dir / 'CMakeLists.txt',), FileFormatter.format_cmakelists)
	format_files((lc_dir / 'config.h.cmake',), FileFormatter.format_cpp)

def format_cpp_files():
	files = chain(*(get_file_paths(lc_dir / sub_dir, 'cpp', 'h')
		for sub_dir in ('src', 'tests')))
	# Exclude C++ files from res subdirectory
	res_dir_pattern = str(lc_dir / 'src' / 'res' / '*')
	files = (p for p in files if not p.match(res_dir_pattern))

	format_files(files, FileFormatter.format_cpp)

def format_tool_files():
	format_files(get_file_paths(lc_dir / 'tools', 'sh'),
		FileFormatter.format_shell)
	format_files(get_file_paths(lc_dir / 'tools', 'py'),
		FileFormatter.format_python)

def format_yml_files():
	format_files((lc_dir / file for file in ('.travis.yml', 'appveyor.yml')),
		FileFormatter.format_cmakelists)

def format_filelists():
	format_files(get_file_paths(lc_dir / 'cmake' / 'filelists', 'txt'),
		FileFormatter.format_filelist)

def format_files(files, fmt_func):
	for file in sorted(files):
		try:
			fmt = FileFormatter(file)
			fmt_func(fmt)
			if not dry_run:
				fmt.save()
		except FormattingError as err:
			print(f'{file}: {err}')

# Recursively list files with specified extensions from specified directory
def get_file_paths(dir_, *exts):
	return chain(*(dir_.glob(f'**/*.{ext}') for ext in exts))

class FileFormatter:
	def __init__(self, file):
		self.file = file
		self.load()

	def format_cmakelists(self):
		self.format_code()
		self.format_left_parenthesis('else', 'elseif', 'endif', 'if')

	def format_cpp(self):
		self.format_code()
		self.format_left_parenthesis('else', 'for', 'if', 'while')

	def format_filelist(self):
		self.format_code()

	def format_python(self):
		self.format_code()

	def format_shell(self):
		self.format_code()

	def format_yml(self):
		self.format_code()

	def save(self):
		with self.file.open('w', encoding='utf-8', newline=NEWLINE) as f:
			f.write(self.s)

	def load(self):
		try:
			with self.file.open(encoding='utf-8', newline=NEWLINE) as f:
				self.s = f.read()
		except UnicodeDecodeError:
			raise FormattingError(
				'Not an UTF-8 encoded file. Please fix manually.')

	def format_code(self):
		self.check_for_non_line_leading_tab()
		self.fix_file_leading_newline()
		self.fix_file_trailing_newline()
		self.trim_line_trailing_horizontal_whitespace()
		self.condense_multiple_empty_lines()

	def check_for_non_line_leading_tab(self):
		if re.search(r'^.*[^\t\n]\t', self.s, re.MULTILINE):
			raise FormattingError(
				'File contains non line leading tab(s). Please fix manually.')

	def condense_multiple_empty_lines(self):
		s = re.sub(r'\n{3,}', r'\n\n', self.s)
		self.update(s, 'Condensed multiple empty lines.')

	def fix_file_leading_newline(self):
		# Remove any newlines at the beginning of the file
		s = re.sub(r'\A[\t\n ]+', '', self.s)
		self.update(s, 'Removed newlines at beginning of file.')

	def fix_file_trailing_newline(self):
		# Make sure file ends in exactly one newline
		s = re.sub(r'[\t\n ]*\Z', r'\n', self.s, 1)
		# Empty file?
		if s == '\n':
			s = ''
		self.update(s, 'Fixed file trailing newline.')

	def format_left_parenthesis(self, *keywords):
		# Enforce space between keywords and following "("
		s = re.sub(fr'\b({"|".join(re.escape(k) for k in keywords)})\(',
			r'\1 (', self.s)
		self.update(s, 'Added space between keyword and left parenthesis.')

	def trim_line_trailing_horizontal_whitespace(self):
		s = re.sub(r'[\t ]+$', '', self.s, flags=re.MULTILINE)
		self.update(s, 'Trimmed trailing horizontal whitespace.')

	def update(self, s, msg):
		if s != self.s:
			self.s = s
			print(f'{self.file}: {msg}')

class FormattingError(Exception):
	pass

if __name__ == '__main__':
	main()
