#!/usr/bin/python
import os
import os.path
import json
import pdb


def get_unknown_detect_data():
    file_time_dict = {}
    unknown_info_file = "unknown_history.txt"
    if not os.path.exists(unknown_info_file):
        return file_time_dict

    for line in open(unknown_info_file).read().strip().split("\n"):
        k, v = line.split(":", 1)
        file_time_dict[k] = v
    return file_time_dict

def get_unknown_label_files():
    cmd = "find ./train/datasets/bbt/ -name \"unknown*.jpg\""
    ret = os.popen(cmd).read().strip()
    if not bool(ret):
        return None
    result = [f.rsplit("/", 2)[-2:] for f in ret.split("\n")]
    result = {}
    for f in ret.split("\n"):
        label, f_name = f.rsplit("/", 2)[-2:]
        if label not in result:
            result[label] = f_name
    return result

def get_poi_data(unknown_label_files, unknown_dict):
    ret = {}
    for label, unknown_img_filename in unknown_label_files.items():
        detect_time = unknown_dict[unknown_img_filename]
        ret[label] = detect_time
    return ret

def main():
    # find ./train/datasets/bbt/ -name "unknown*.jpg"
    #./train/datasets/bbt/jong/unknown0000000001.jpg
    unknown_label_files = get_unknown_label_files()
    if not bool(unknown_label_files):
        return

    unknown_dict = get_unknown_detect_data()

    poi_data = get_poi_data(unknown_label_files, unknown_dict)

    poi_info_file = "poi_info_file.txt"
    with open(poi_info_file, "w") as f:
        f.write(json.dumps(poi_data, indent=4))

if __name__ == "__main__":
    main()
