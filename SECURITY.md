# Security Policy

## Supported Versions

Security updates are provided for the latest version available in the `main` branch.

| Version | Supported          |
| ------- | ------------------ |
| main    | ✅ Yes             |
| Older releases | ❌ No      |

## Reporting a Vulnerability

The security of WebSearchEngine is taken seriously. If you discover a security vulnerability, please report it responsibly.

### Please Do Not

- Open a public GitHub issue for security vulnerabilities.
- Disclose vulnerability details publicly before a fix has been released.

### How to Report

Please contact the maintainer directly with the following information:

- Description of the vulnerability
- Steps to reproduce the issue
- Potential impact
- Proof-of-concept code, screenshots, or logs (if available)
- Suggested mitigation or fix (optional)

You can contact the project maintainer through the GitHub profile associated with this repository:

- Repository: [WebSearchEngine](https://github.com/mihaimoga/WebSearchEngine)
- Maintainer: [@mihaimoga](https://github.com/mihaimoga)

After a vulnerability report is received:

1. The report will be reviewed and acknowledged as soon as possible.
2. The vulnerability will be validated and assessed.
3. A fix will be developed and tested.
4. A security release will be published when appropriate.
5. Credit may be given to the reporter unless anonymity is requested.

## Scope

This policy applies to:

- The C++ desktop application source code
- Supporting scripts and utilities included in the repository
- Configuration files and build artifacts maintained in the project

Third-party libraries and dependencies should be reported to their respective maintainers unless the issue arises from their integration within this project.

## Security Best Practices

When building or deploying WebSearchEngine:

- Use the latest supported version of Visual Studio and compiler toolchain.
- Keep all third-party dependencies updated.
- Validate and sanitize all external input.
- Follow the principle of least privilege when configuring database connections or deployment environments.
- Restrict access to any credentials, connection strings, or sensitive configuration data.

Thank you for helping keep WebSearchEngine and its users secure.
