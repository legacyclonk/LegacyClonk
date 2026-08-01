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
                "runner": runner["runner"],
                **config,
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


def print_filtered_matrix(toml: dict, output_name: str, filter_func: collections.abc.Callable[[dict], bool]):
    result = transform_matrix(toml["matrix"])
    result = list(filter(filter_func, result))
    print(f"{output_name}={json.dumps(result)}")

if __name__ == "__main__":
    isRelease = False
    if len(sys.argv) > 2:
        isRelease = sys.argv[2] == "true"

    toml = tomllib.load(open(sys.argv[1], "rb"))
    print_output(toml, "groups")
    print_output(toml, "major-update", default=False)
    print(f"butler-dry-run={json.dumps(toml.get('itch.io', {}).get('dry-run', False))}")
    print(f"matrix-itch={json.dumps(transform_matrix_itch(toml["matrix"]))}")
    print_filtered_matrix(toml, "matrix", lambda entry: not entry.get("publish-only", False))
    print_filtered_matrix(toml, "matrix-release-build",
                          lambda entry:
                              not entry.get("exclude-release", False)
                              and not entry.get("publish-only", False))
    print_filtered_matrix(toml, "matrix-release-publish",
                          lambda entry:
                              not entry.get("exclude-release", False)
                              and not entry.get("build-only", False))
