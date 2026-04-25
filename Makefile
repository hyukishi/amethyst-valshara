.PHONY: all docker-build docker-shell docker-run docker-up docker-make docker-stop native-build worlddb-refresh web-ui

all: docker-up

docker-build:
	docker compose build --no-cache --pull mud

docker-shell:
	docker compose run --rm --workdir /workspace mud bash

docker-run:
	docker compose run --rm --service-ports --workdir /workspace/src mud ./smaug 4000

docker-up:
	docker compose up --build mud

docker-make:
	docker compose run --rm --workdir /workspace mud make -C src

docker-stop:
	docker compose down --remove-orphans

native-build:
	make -C src

worlddb-refresh:
	python3 scripts/migrate_worlddb.py

web-ui:
	python3 webui/server.py
