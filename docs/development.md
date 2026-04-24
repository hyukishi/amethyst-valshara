# Development Workflow

## Docker Compose

The preferred development workflow is now container-based.

### Build the image

```sh
make docker-build
```

### Open a shell in the dev container

```sh
make docker-shell
```

### Run the MUD on port 4000 from the container

```sh
make docker-run
```

This publishes port `4000` from the container to port `4000` on the host.

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
