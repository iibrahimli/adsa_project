#! /bin/bash

print_help(){
    echo "Usage: $0 <option> [<opt_args>]"
    echo "Options:"
    echo "  -p <img_dir> <psf_dir>                 :  Calculate psf values for each image in <img_dir>"
    echo "                                            and write psf profiles to <psf_dir>"
    echo ""
    echo "    *** Note: psf profile of an image 'example.bmp' is stored as 'example.bmp.psf'"
    echo ""
    echo ""
    echo "  -s <img_dir> <psf_dir> <resized_dir>   :  Scale all images in img_dir according to their psf profile"
    echo "                                            and save them under name 'xxx_resized.bmp' in <resized_dir>"
    echo ""
    echo ""
    echo "  -m <img_dir> <out_img>                 :  Merge all images in given directory and save result as <out_img>"
    echo ""
    echo ""
    echo "  -f <img_dir> <out_img>                 :  Fully automatic, do everything to get <out_img> from <img_dir>"
    echo "                                            uses 'temp/psf' to store psf files and 'temp/resized' for intermediate results"
    echo ""
    echo ""
    echo "  -h                                     :  Display this"
    echo ""
}


if [ $# -ge 3 ]
then
    case $1 in
    -p)
        # psf
        img_dir="$2"
        psf_dir="$3"

        mkdir -p $psf_dir

        n_files=($(ls -1 $img_dir | wc -l))
        files=($(ls -1 $img_dir | sort -V))

        echo "[*] calculating psf"

        for ((i=0; i<n_files-1; i++)); do
            build/flt --psf "$img_dir/${files[$i]}" "$img_dir/${files[$((i+1))]}" "$psf_dir/${files[$i]}.psf"
            echo -ne "\rCalculating psf: $((i+1))/$n_files"
        done
        build/flt --psf "$img_dir/${files[$((n_files-1))]}" "$img_dir/${files[0]}" "$psf_dir/${files[$((n_files-1))]}.psf"
        echo -ne "\rCalculating psf: $n_files/$n_files"
        echo ""
        echo "[.] done calculating psf"
        ;;

    -s)
        # scale
        img_dir="$2"
        psf_dir="$3"
        resized_dir="$4"

        mkdir -p $resized_dir

        echo "[*] scaling"

        n_files=($(ls -1 $img_dir | wc -l))
        a=1

        for file in $(ls -1 $img_dir | sort -V)
        do
            echo -ne "\rScaling: $a/$n_files"
            build/flt --scale "$img_dir/$file" "$psf_dir/$file.psf" "$resized_dir/${file%.bmp}_resized.bmp"
            let a++
        done
        echo ""
        echo "[.] done scaling"
        ;;

    -m)
        # merge
        img_dir="$2"
        out_img="$3"

        args=""
        for file in $(ls -1 $img_dir | sort -V)
        do
            args="$args $img_dir/$file"
        done

        echo "[*] merging"

        build/flt --merge "$out_img" $args

        echo "=== DONE ==="
        ;;

    -f)
        # full automatic
        img_dir="$2"
        psf_dir="temp/psf"
        resized_dir="temp/resized"
        out_img="$3"

        rm -rf "$psf_dir" "$resized_dir"

        mkdir -p $psd_dir $resized_dir

        $0 -p "$img_dir" "$psf_dir" && $0 -s "$img_dir" "$psf_dir" "$resized_dir" && $0 -m "$resized_dir" "$out_img"
        ;;

    *)
        # wrong
        print_help
        ;;
    esac

else
    print_help
fi
