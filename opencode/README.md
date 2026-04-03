# OpenCode Docker Development Environment

A containerized development environment for [OpenCode](https://opencode.ai).

## Prerequisites

- Docker
- Docker Compose
- GNU Make

## Quick Start

```bash
make run
```

## Available Commands

| Command | Description |
|---------|-------------|
| `make build` | Build the Docker image |
| `make run` | Run the container (default) |

## How It Works

- The container mounts your workspace (`../../:/workspace`) for live development
- User UID/GID are automatically matched to your host user
- Shell is configured with custom prompt and `ll` alias
