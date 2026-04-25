# Development Workflow

## Docker Compose

The preferred development workflow is now container-based.

### Build the image only

```sh
make docker-build
```

### Build and run the MUD

```sh
make
```

This is the default target now. It builds the image if needed and starts the
`smaug` server on port `4000`.

### Open a shell in the dev container

```sh
make docker-shell
```

### Run the MUD on port 4000 from the container

```sh
make docker-run
```

This launches a one-off container and runs `./src/smaug 4000` directly.
For the normal long-running workflow, prefer `make`.

### Rebuild the game binary inside the container

```sh
make docker-make
```

### Stop the compose service

```sh
make docker-stop
```

## Notes

- The repository is bind-mounted into `/workspace` inside the container.
- Local runtime data such as `player/`, `gods/`, and SQLite database files remain on the host filesystem through the bind mount.
- The image itself excludes runtime-generated data via `.dockerignore` so image builds stay smaller and cleaner.
- If Docker is not installed on the host yet, install Docker Engine plus the Compose plugin first.

## Native fallback

If you need to build without Docker for a quick local check:

```sh
make native-build
```

That rebuilds the `src/` tree directly on the host.
