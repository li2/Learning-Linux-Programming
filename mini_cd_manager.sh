#! /bin/bash

menu_choice=""
current_cd=""
readonly TITLE_FILE="title.cdb"
readonly TRACKS_FILE="tracks.cdb"
temp_file=/tmp/cdb.$$
trap 'rm -f $temp_file' EXIT


set_menu_choice() {
    clear
    echo "Options :-"
    echo
    echo "    a) Add new CD"
    echo "    f) Find CD"
    echo "    c) Count the CDs and tracks in the catalog"
    if [ "$cdcatnum" != "" ]; then
	echo "        l) List tracks on $cdtitle"
	echo "        r) Remove $cdtitle"
	echo "        u) Update track information for $cdtitle"
    fi
    echo "    q) Quit"
    echo
    echo -e "Please enter choice then press return \c"
    read menu_choice
    return
}

get_return(){
    echo -e "Press return \c"
    read x
    return 0
}

get_confirm(){
    echo -e "Are you sure? \c"
    while true
    do 
	read x
	case "$x" in 
	    y | yes | Y | Yes | YES )
		return 0
		;;
	    n | no | N | No | NO )
		echo
		echo "Cancelled"
		return 1
		;;
	    * )
		echo "Please enter yes or no"
		;;
	esac
    done
}

insert_title(){
    echo $* >> $TITLE_FILE
    return
}

insert_track(){
    echo $* >> $TRACKS_FILE
    return
}

add_records(){
    # Prompt for the initial information
    
    echo -e "Enter catalog name \c"
    read tmp
    cdcatnum=${tmp%%,*}

    echo -e "Enter title \c"
    read tmp
    cdtitle=${tmp%%,*}

    echo -e "Enter type \c"
    read tmp
    cdtype=${tmp%%,*}

    echo -e "Enter artist/composer \c"
    read tmp
    cdac=${tmp%%,*}

    # Check that they want to enter the information
    
    echo "About to add new entry"
    echo "$cdcatnum $cdtitle $cdtype $cdac"

    # If confirmed then append it to the titles file

    if get_confirm ; then
	insert_title $cdcatnum, $cdtitle, $cdtype, $cdac
	add_record_tracks
    else
	remove_records
    fi

    return
}

add_record_tracks(){
    echo "Enter track information for this CD"
    echo "When no more tracks enter q"
    cdtrack=1
    cdttitle=""
    while [ "$cdttitle" != "q" ]
    do 
	echo -e "Track $cdtrack, track title? \c"
	read tmp
	cdttitle=${tmp%%,*}
	if [  ]; then
	    echo "Sorry, no commands allowed"
	    continue
	fi
	if [ -n "$cdttitle" ]; then
	    if [ "$cdttitle" != "q" ]; then
		insert_track $cdcatnum, $cdtrack, $cdttitle
	    fi
	else
	    cdtrack=$((cdtrack-1))
	fi
	cdtrack=$((cdtrack+1))	
    done
}

find_cd(){
    if [ "$1" = "n" ] ; then
	asklist=n
    else
	asklist=y
    fi 

    cdcatnum=""
    echo -e "Enter a string to search for in the CD titles \c"
    read searchstr
    if [ "$searchstr" = "" ] ; then
	return 0
    fi 

    grep "$searchstr" $TITLE_FILE > $temp_file
    
    set $(wc -l $temp_file)
    linesfound=$1

    case "$linesfound" in 
    0)      echo "Sorry, nothing found"
	    get_return
	    return 0
	    ;;
    1)      ;;
    2)      echo "Sorry, not unique."
	    echo "Found the following"
	    cat $temp_file
	    get_return
	    return 0
	    ;;
    esac

    IFS=","
    read cdcatnum cdtitle cdtype cdac < $temp_file
    IFS=""

    if [ -z "$cdcatnum" ] ; then
	echo "Sorry, could not extract catalog field from $temp_file"
	get_return
	return 0
    fi 

    echo
    echo Catalog number: $cdcatnum
    echo Title: $cdtitle
    echo Type: $cdtype
    echo Artist/Composer: $cdac
    echo
    get_return

    if [ "$asklist" = "y" ] ; then
	echo -e "View tracks for this CD? \c"
	read x
	if [ "$x" = "y" ] ; then
	    echo
	    list_tracks
	    echo
	fi 
    fi 
    return 1
}

update_cd(){
    if [ -z "$cdcatnum" ] ; then
	echo "You must select a CD first"
	find_cd n
    fi 

    if [ -n "$cdcatnum" ] ; then
	echo "Current tracks are :-"
	list_tracks
	echo 

	echo "This will re-enter the tracks for $cdtitle"
	get_confirm && {
	    grep -v "^${cdcatnum}," $TRACKS_FILE > $temp_file
	    mv $temp_file $TRACKS_FILE
	    echo
	    add_record_tracks
	}
    fi 
    return
}

count_cds(){
    set $£¨wc -l $TITLE_FILE£©
    num_titles=$1
    set $£¨wc -l $TRACKS_FILE£©
    num_tracks=$1
    echo "found $num_titles CDs, with a total of $num_tracks"
    get_return
    return
}

remove_records(){
    if [ -z "$cdcatnum" ] ; then
	echo You must select a CD first
	find_cd n
    fi

    if [ -n "$cdcatnum" ] ; then
	echo "You are about to delete $cdtitle"
	get_confirm && {
	    grep -v "^${cdcatnum}," $TITLE_FILE > $temp_file
	    mv $temp_file $TITLE_FILE
	    grep -v "^${cdcatnum}," $TRACKS_FILE > $temp_file
	    mv $temp_file $TRACKS_FILE
	    cdcatnum=""
	    echo Entry removed
	    add_record_tracks
	}
	get_return
    fi
    return
}

list_tracks(){
    if [ "$cdcatnum" = "" ] ; then
	echo no CD selected yet
	return
    else
        grep "^${cdcatnum}," $TRACKS_FILE > $temp_file
	num_tracks=$(wc -l $temp_file)
	if [ "$num_tracks" = "0" ] ; then
	    echo no tracks found for $cdtitle
        else {
	    echo
	    echo "$cdtitle :-"
	    echo
	    cut -f 2- -d , $temp_file
	    echo
	} | ${PAGER:-more}
	fi
    fi
    get_return
    return
}


# main

rm -f $temp_file
if [ ! -f $TITLE_FILE ]; then
    touch $TITLE_FILE
fi
if [ ! -f $TRACKS_FILE ]; then
    touch $TRACKS_FILE
fi

clear
echo
echo
echo "Mini CD Manager"
sleep 1

quit=n
while [ "$quit" != "y" ]
do
    set_menu_choice
    case "$menu_choice" in
	a) add_records
	    ;;
	r) remove_records
	    ;;
	f) find_cd y
	    ;;
	u) update_cd
	    ;;
	c) count_cds
	    ;;
	l) list_tracks
	    ;;
	b)  echo
	    more $TITLE_FILE
	    echo
	    get_return
	    ;;
	q|Q) quit
	    ;;
	*) echo "Sorry, choice not recognized"
	    ;;
    esac
done

# tidy up and leave

rm -f $temp_file
echo "Finished"
exit 0
