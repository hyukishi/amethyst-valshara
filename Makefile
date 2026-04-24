.PHONY: docker-build docker-shell docker-run docker-make docker-stop native-build

docker-build:
	docker compose build mud

docker-shell:
	docker compose run --rm mud bash

docker-run:
	docker compose run --rm --service-ports mud ./src/smaug 4000

docker-make:
	docker compose run --rm mud make -C src

docker-stop:
	docker compose down --remove-orphans

native-build:
	make -C src
