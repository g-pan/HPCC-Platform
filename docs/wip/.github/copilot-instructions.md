# HPCC Systems Documentation Workspace - AI Coding Guidelines

## Architecture Overview

This workspace contains the documentation build system for **HPCC Systems**, a massive parallel processing platform for big data analytics. The platform centers around **ECL (Enterprise Control Language)** - a declarative, non-procedural language designed for data-intensive applications.

### Core Components
- **ECL Language**: Declarative data processing language with dictionary-style definitions
- **Thor**: Parallel batch processing cluster 
- **Roxie**: Real-time query processing cluster
- **Dali**: Distributed metadata/configuration server
- **ESP (Enterprise Services Platform)**: Web services interface layer

## Key Conventions & Patterns

### Documentation Structure
```
docs/wip/
├── ECLLanguageReference/    # ECL language documentation modules
├── ECLProgrammersGuide/     # Programming guide modules  
├── EX-DEV-FILES/           # Example/development files
└── HTML-DEV/               # HTML generation artifacts
```

### DocBook XML Documentation
- All documentation uses **DocBook XML 4.5** format
- Modular approach: large documents split into `*_mods/` subdirectories
- Each module is a self-contained XML file (e.g., `BltInFunc-OUTPUT.xml`)
- Build system uses CMake + XSLT for transformations

### ECL Language Specifics
- **Definitions vs Actions**: ECL code is either data definitions (reusable expressions) or executable actions
- **Non-procedural**: Execution order determined by dependencies, not code order
- **Dictionary approach**: Each definition can reference previous definitions, extending the language
- **Import pattern**: `IMPORT LibraryName as Alias;` for external libraries

### Build System Patterns
```cmake
# Standard pattern in CMakeLists.txt
get_filename_component(DOC_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../BuildTools/cmake_config/${DOC_DIR_NAME}.txt)
```

### XSLT Transformation Pipeline
- `fo.xsl`/`fo.xsl.in`: XSL-FO transformations for PDF generation
- `DOCBOOK_TO_PDF()`: CMake macro for PDF document generation  
- `DOCBOOK_TO_HTML()`: CMake macro for HTML documentation
- Multiple output formats: PDF, HTML, Eclipse Help, Portal HTML

## Development Workflows

### Documentation Build Process
1. **XML Validation**: DocBook XML files must validate against DTD
2. **XSLT Processing**: Transform XML → HTML/PDF using custom stylesheets
3. **CMake Integration**: Use `MAKE_DOCS` flag to control documentation builds
4. **Multi-language Support**: `DOC_LANG` variable for internationalization

### Working with ECL Examples
- ECL syntax uses **declarative definitions**: `IMPORT`, `EXPORT`, `DATASET`, `OUTPUT`
- **Comments**: Use `//` for line comments, `//* @param` for JSDoc-style parameter docs
- **Actions vs Expressions**: Only actions (like `OUTPUT`) execute; definitions are reusable expressions

### XSLT Development Patterns
```xsl
<!-- Standard HPCC copyright header in all XSL files -->
<!-- Apache License 2.0 with HPCC Systems copyright -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
```

## Critical Integration Points

### Configuration System
- XML-based configuration files with XSL transformations
- Component configuration: `eclccserver.xsl`, `eclscheduler.xsl`, `esp.xsl`
- Template system for generating component configurations

### Code Generation
- ESDL (Enterprise Service Description Language) for service definitions
- XSLT-based code generation: `esdl2cpp_*.xslt`, `esdl2java_*.xslt`, `esdl2ecl.xslt`
- Service interface generation from XML schemas

## Common Anti-Patterns to Avoid

- **Don't treat ECL like procedural code** - it's declarative and non-procedural
- **Don't break DocBook XML validation** - always validate against DTD 4.5
- **Don't hardcode paths** - use CMake variables like `${CMAKE_CURRENT_SOURCE_DIR}`
- **Don't skip HPCC copyright headers** - all files need Apache 2.0 + HPCC Systems copyright

## Quick Reference Files

- `ECLR-includer.xml`: Main ECL Language Reference document structure
- `fo.xsl.in`: Primary XSL-FO transformation template  
- `CMakeLists.txt`: Build configuration entry points
- `ECLR_mods/BltInFunc-*.xml`: ECL built-in function documentation modules
- `DaliAdmin.xml`: Example of system administration documentation structure

## External Resources

- **Main HPCC-Platform Repository**: https://github.com/hpcc-systems/HPCC-Platform
  - Complete platform source code, build instructions, and development guidelines
  - Issue tracking and contribution workflows
  - Release notes and platform architecture documentation
  
- **Official ECL Language Reference**: https://hpccsystems.com/wp-content/uploads/_documents/ECLR_EN_US/
  - Authoritative reference for ECL syntax, built-in functions, and language semantics
  - Cross-reference documentation changes against the published reference to ensure consistency

- **ECL Programmer's Guide**: https://hpccsystems.com/wp-content/uploads/_documents/ProgrammersGuide_EN_US/
  - Comprehensive programming guide with practical examples and best practices
  - Essential for understanding ECL programming patterns and workflow development
  - Complement to the language reference for real-world ECL development

- **ECL Standard Library Reference**: https://hpccsystems.com/wp-content/uploads/_documents/SLR_EN_US/
  - Complete documentation of ECL standard library modules and functions
  - Reference for Std library functions, ML (Machine Learning) library, and other standard modules
  - Essential for understanding available pre-built functionality and library usage patterns

When working with this codebase, remember that documentation quality directly impacts developer experience with the HPCC platform - accuracy and completeness are critical for this enterprise-grade big data system.
