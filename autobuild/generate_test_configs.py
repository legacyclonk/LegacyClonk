#! /usr/bin/env python3

import json
import sys
import tomllib
import typing
import itertools


TomlValue = typing.Union[dict, str, int, float, bool, list]


def generate_variants(*variants: list[list[dict]]):
    configs = []
    for variant in itertools.product(*filter(lambda scope: scope is not None, variants)):
        merged = {}
        for scope in variant:
            merged = {
                **merged,
                **scope
            }
        configs.append(" ".join(f"-D{key}={"ON" if val else "OFF"}"
                                for key, val in merged.items()))

    return configs


def transform_matrix(general: dict, matrix: dict) -> list:
    general_variants = general["variants"]
    del general["variants"]
    general_base = [general]

    new_matrix = []
    for os, runner in matrix.items():
        os_variants = runner["variants"] if "variants" in runner else None

        all_variants = generate_variants(general_base, general_variants, os_variants)

        for config in runner["config"]:
            config_suffix = f"{os.capitalize()}-{config["arch"]}"
            platform_suffix = config["platform-suffix"]
            try:
                config_suffix = f"{config_suffix}-{config["name"]}"
                platform_suffix = f"{platform_suffix}-{config["name"]}"
            except KeyError:
                pass

            for config_args in all_variants:
                new_matrix.append({
                    "runner": runner["runner"],
                    **config,
                    "os": os.capitalize(),
                    "config-suffix": config_suffix,
                    "platform-suffix": platform_suffix,
                    "extra-config-args": config_args
                })
    return new_matrix


if __name__ == "__main__":
    toml = tomllib.load(open(sys.argv[1], "rb"))
    print(f"matrix={json.dumps(transform_matrix(toml.get("general"), toml.get("matrix")))}")
