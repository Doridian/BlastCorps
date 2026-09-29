MODULES = ["init", "hd_code", "hd_front_end"]
VERSIONS = ["us.v10", "us.v11", "jp", "eu"]


def add_custom_arguments(parser):
    parser.add_argument("--module", choices=MODULES, default="init",
                        help="Code module the function lives in.")
    parser.add_argument("--version", dest="game_version", choices=VERSIONS,
                        default="us.v11", help="Game version.")


def apply(config, args):
    name = f"{args.module}.{args.game_version}"
    config["baseimg"] = f"{name}.bin"
    config["myimg"] = f"build/{name}.bin"
    config["mapfile"] = f"build/{name}.map"
    config["source_directories"] = ["src", "include"]
    config["makeflags"] = [f"VERSION={args.game_version}"]
