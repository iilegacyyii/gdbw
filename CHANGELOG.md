
# Change Log
All notable changes to this project will be documented in this file.
 
The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [0.1.1] - 2025-08-28

### Added

- `breakpointenable` & `breakpointdisable` commands
- GetVMRegion binding
- Version resource to help with Anti-Virus signatures
- This CHANGELOG file

### Changed

- `help` command now displays alias list
- `.gitignore` to ignore `.7z` and `.res` files

### Fixed

- `examine` command's improper `--count` flag handling.
- `delbreakpoint` command nullptr dereference when deleting an already deleted breakpoint.

## [0.1.0] - 2025-08-27

### Added

- Core debugging engine
- Disassembly
- Plugin System

[0.1.1]: https://github.com/iiLegacyyii/gdbw/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/iiLegacyyii/gdbw/releases/tag/v0.1.0