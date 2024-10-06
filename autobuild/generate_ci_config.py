#! /usr/bin/env python3

import collections.abc
import datetime
import json
import sys
import tomllib
import typing


TomlValue = typing.Union[dict, str, int, float, bool,
                         datetime.datetime, datetime.date, datetime.time, list]


def transform_matrix(matrix: dict) -> list:
    new_matrix = []
    for os, runner in matrix.items():
        for config in runner["config"]:
            config_suffix = f"{os.capitalize()}-{config["arch"]}"
            platform_suffix = config["platform-suffix"]
            try:
                config_suffix = f"{config_suffix}-{config["name"]}"
                platform_suffix = f"{platform_suffix}-{config["name"]}"
            except KeyError:
                pass

            new_matrix.append({
                **config,
                "runner": runner["runner"],
                "os": os.capitalize(),
                "config-suffix": config_suffix,
                "platform-suffix": platform_suffix
            })
    return new_matrix

def transform_matrix_itch(matrix: dict) -> list:
    new_matrix = []
    for os, runner in matrix.items():
        for config in runner["config"]:
            if "itch-manifest" in config:
                new_matrix.append({
                    "runner": runner["runner"],
                    "os": os.capitalize(),
                    **config
                })
    return new_matrix


def print_output(toml: dict, name: str, *, transform: collections.abc.Callable[[TomlValue], TomlValue] = lambda x: x, **kwargs):
    value = None
    if "default" in kwargs:
        value = toml.get(name, kwargs["default"])
    else:
        value = toml[name]
    print(f"{name}={json.dumps(transform(value))}")


if __name__ == "__main__":
    toml = tomllib.load(open(sys.argv[1], "rb"))
    print_output(toml, "groups")
    print_output(toml, "major-update", default=False)
    print_output(toml, "matrix", transform=transform_matrix)
    print(f"matrix-itch={json.dumps(transform_matrix_itch(toml["matrix"]))}")
