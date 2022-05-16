for file in ../traces/*
do
    echo $file
    echo ----------
    for predictor in "gshare" "tournament" "custom"
    do
        echo $predictor
        echo ~~~~~~
        bunzip2 -kc $file | ./predictor --$predictor
        echo 
    done
done