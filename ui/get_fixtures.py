# This script is used to update the fixtures for the unit tests. Point it at a running BrewPi device and it will
# access that device's API endpoints and update the fixtures with the latest data. This is useful when detecting if
# the API has changed in a way that breaks the unit tests (and therefore presumably the app functionality).

import requests
import os
import json
import shutil

from typing import List
from argparse import ArgumentParser

def compare_keys(old_data: dict, new_data: dict) -> (List[str], List[str]):
    old_keys = set(old_data.keys())
    new_keys = set(new_data.keys())
    added = new_keys - old_keys
    removed = old_keys - new_keys
    return list(added), list(removed)

def compare_types(old_data: dict, new_data: dict) -> List[str]:
    common_keys = set(old_data.keys()).intersection(set(new_data.keys()))
    changed = [key for key in common_keys if type(old_data[key]) != type(new_data[key])]
    return changed

def main(host: str):
    fixture_dir = 'tests/stores/fixtures'
    for filename in os.listdir(fixture_dir):
        if filename.endswith('.json'):
            endpoint = filename[:-5]  # strip off .json
            endpoint = endpoint.replace('.', '/')
            url = f'http://{host}/{endpoint}/'
            try:
                response = requests.get(url)
            except:
                print(f'Could not connect to {url}')
                continue
            new_data = response.json()

            filepath = os.path.join(fixture_dir, filename)
            with open(filepath, 'r') as f:
                old_data = json.load(f)

            print(f'Endpoint: {endpoint}')

            if isinstance(old_data, dict) and isinstance(new_data, dict):
                added, removed = compare_keys(old_data, new_data)
                changed = compare_types(old_data, new_data)
                print(f'Keys added: {added}')
                print(f'Keys removed: {removed}')
                print(f'Types changed: {changed}')
            else:
                print('Data is not a dict - cannot compare keys or types')


            # Backup the old file
            backup_path = filepath + '.backup'
            if os.path.exists(backup_path):
                os.remove(backup_path)
            shutil.move(filepath, backup_path)

            # Write the new data
            with open(filepath, 'w') as f:
                json.dump(new_data, f, indent=4)

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('host', type=str, help='Hostname or IP address')
    args = parser.parse_args()
    main(args.host)
