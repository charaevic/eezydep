#!/usr/bin/env python3
import argparse
import os
import sys
import signal
import subprocess
#TODO: add help descriptions
path = "./config/routes.conf"
parser = argparse.ArgumentParser(prog='eezydep')
sub = parser.add_subparsers(dest="command")

route = sub.add_parser("route")
route_sub = route.add_subparsers(dest="action")

route_sub.add_parser("list")

add = route_sub.add_parser("add")
add.add_argument("hostname")
add.add_argument("backend")

rm = route_sub.add_parser("remove")
rm.add_argument("hostname")

logs = sub.add_parser("logs")
status = sub.add_parser("status")
start = sub.add_parser("start")
stop = sub.add_parser("stop")
restart = sub.add_parser("restart")
reload = sub.add_parser("reload")
args = parser.parse_args()
if args.command is None:
    parser.print_help()
    sys.exit(1)

if args.command == "route":

    if args.action == "list":
        try:
            with open(path, 'r') as conf:
                for line in conf:
                    print(line.rstrip())
        except FileNotFoundError:
            print("Config file not found")
            sys.exit(1)


    if args.action == "add":

        try:
            with open(path, 'a') as conf:
                conf.write(args.hostname + " " + args.backend + "\n")
            subprocess.run(["pkill", "-HUP", "proxy"])
        except FileNotFoundError:
            print("Config file not found")
            sys.exit(1)

    if args.action == "remove":
        with open(path, 'r') as conf:
            content = conf.readlines()
        with open(path+".tmp", "w") as conf:
            for line in content:
                if not line.startswith(args.hostname):
                    conf.write(line)

        os.replace(path + ".tmp", path)

if args.command == "status":
    result = subprocess.run(["pgrep", "proxy"], capture_output=True, text=True)
    if result.returncode == 0:
        print(f"Proxy: running (PID {result.stdout.strip()})\n")
    else:
        print("Proxy: not running")

    with open(path, 'r') as conf:
        lines = len(conf.readlines())
        print(f"Loaded {lines} route configurations")

if args.command == "logs":
    subprocess.run(["journalctl", "-u", "eezydep", "-f"])

#Shortcuts
if args.command == "start":
    subprocess.run(["sudo", "systemctl", "start", "eezydep"])

if args.command == "stop":
    subprocess.run(["sudo", "systemctl", "stop", "eezydep"])
if args.command == "restart":
    subprocess.run(["sudo", "systemctl", "restart", "eezydep"])
if args.command == "reload":
    subprocess.run(["pkill", "-HUP", "proxy"])
                
            


