.DEFAULT_GOAL := run

UID = $(shell id -u ${USER})
GID = $(shell id -g ${USER})

.PHONY: build run

build:
	docker compose build --progress=plain

run:
	docker compose run -it --env UID=${UID} --env GID=${GID} opencode
