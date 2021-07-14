rm svm/*
#rm engines/*
rm temp.prototxt
rm -rf faces/train/cropped
rm -rf faces/info.txt
rm -rf faces/labels.txt
cd faces
python3 generate_training_data.py train/datasets/bbt/

# generate PoI information
python3 generate_person_of_interest_info.py
