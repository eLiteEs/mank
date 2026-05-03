# mank
A local-first version control system written in C++, inspired by Git with built-in tools for releases, packaging and local CI/CD pipelines.

## Features
- Full version control: add, commit, branch, merge, stash, diff, log...
- Tagged releases with source zip, changelog and HTML summary
- Local CI/CD pipelines triggered by commits, merges or releases
- Pack and unpack repositories for backup or distribution
- Submodule support for linking other mank repositories
- Built-in manual: `mank man <command>`

## Dependencies
- cmake
- g++ (C++17)
- zlib
- openssl
- libzip
- yaml-cpp

On Ubuntu/Debian:
```bash
sudo apt install cmake g++ zlib1g-dev libssl-dev libzip-dev libyaml-cpp-dev
```

## Building
```bash
cmake -B build
cmake --build build
```

The executable will be at `build/mank`.

Optionally, install it system-wide:
```bash
sudo cp build/mank /usr/local/bin/
```

## Quick start
```bash
mank init                          # initialize a repository
mank --config.name "Your Name"     # set your name
mank --config.email "you@mail.com" # set your email

mank add .                         # stage all files
mank commit "first commit"         # create a commit
mank log                           # view history
mank diff .                        # view changes
```

## Commands
Run `mank help` for a quick reference of all commands.

For detailed documentation on any command:
```bash
mank man <command>

# Examples:
mank man diff
mank man ci
mank man release
```

## CI/CD
Create `.mank/ci/pipeline.yml` to define your pipelines:

```yaml
on:
  - commit
  - merge
  - manual

jobs:
  build:
    run: make

  test:
    steps:
      - run: make test
      - run: ./run_tests.sh
    depends:
      - build
```

Jobs run in the background and send a system notification when they finish.
View logs with `mank ci logs` or `mank ci logs <job>`.

## License
GNU GPL v3 — see the `LICENSE` file for details.