if [ ! -d "$3" ]; then
    mkdir -p "$3"
    if [ $? -ne 0 ]; then
        echo "create directory $3 failed"
        exit 1
    fi
fi

valgrind --tool=memcheck --xml=yes --xml-file=$1 --gen-suppressions=all --leak-check=full --leak-resolution=med --track-origins=yes --vgdb=no $2