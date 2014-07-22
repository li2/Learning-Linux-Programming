#! /bin/bash

menu_choicea=""
current_cd=""
readonly TITLE_FILE="title.cdb"
readonly TRACKS_FILE="tracks.cdb"
# 以进程编号命名临时文件
temp_file=/tmp/cdb.$$
trap 'rm -f $temp_file' EXIT


set_menum_choice() {
    clear
    echo "Options :-"
    echo
    echo "    a) Add new CD"
    echo "    f) Find CD"
    echo "    c) Count the CDs and tracks in the catalog"
    # cdcatnum记录当前增加或查找的CD目录编号，add/find/remove会更新该值
    # 不为空时，需要扩展菜单选项
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

# get_confirm 函数用于提示用户确认操作，会被多次调用，所以封装为函数
#   1)  if get_confirm ; then
#   2)  get_confirm && {}
# if exit code is 0 (means true), runs the then branch;
# if exit code is non-0 (means false), the then branch will be skipped.
# http://stackoverflow.com/questions/2699237/bourne-if-statement-testing-exit-status
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

# * 为当前进程的所有命令行参数。加了双引号"$*", 相当于字符串。
insert_title(){
    echo $* >> $TITLE_FILE
    return
}

insert_track(){
    echo $* >> $TRACKS_FILE
    return
}

# ${variable%%pattern} 模式匹配运算：
# 如果pattern匹配于variable的结尾处，删除匹配的最长部分，并返回剩余部分。
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
        # 以逗号作为字段分隔符，所以不要有空格
	insert_title $cdcatnum,$cdtitle,$cdtype,$cdac
	add_record_tracks
    else
	remove_records
    fi

    return
}

add_record_tracks(){
    echo "Enter track information for this CD"
    echo "When no more tracks enter q"
    cdtracknum=1
    cdtracktitle=""
    while [ "$cdtracktitle" != "q" ]
    do 
	echo -e "Track $cdtracknum, input track title: \c"
	read tmp
	cdtracktitle=${tmp%%,*}
        # 输入的名称不能包含字段分隔符逗号；
	if [ "$tmp" != "$cdtracktitle" ]; then
	    echo "Sorry, no commands allowed"
	    continue
	fi
        # 如果输入的名称不为空，且不是表示退出的q，那么写入到文件；
	if [ -n "$cdtracktitle" ]; then
	    if [ "$cdtracktitle" != "q" ]; then
		insert_track $cdcatnum,$cdtracknum,$cdtracktitle
                cdtracknum=$((cdtracknum+1))
	    fi
	fi
    done
}

find_cd(){
    # 先检查命令行参数$1，当查找到对应的CD后，是否询问用户，以列出CD曲目；
    if [ "$1" = "n" ] ; then
	asklist=n
    else
	asklist=y
    fi 

    # CD titles file包含4个字段，可以查询任意字段；
    # 首先检查用户输入的字串是否为空；
    cdcatnum=""
    echo -e "Enter a string to search for in the CD titles \c"
    read searchstr
    if [ "$searchstr" = "" ] ; then
	return 0
    fi 

    # 然后调用grep查找匹配的行，并重定向到临时文件；
    grep "$searchstr" $TITLE_FILE > $temp_file

    # 统计匹配的行数；
    set $(wc -l $temp_file)
    linesfound=$1

    # 如过匹配的行数>1，
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

    # 读取匹配的第一行，以获取CD信息；
    IFS=","
    read cdcatnum cdtitle cdtype cdac < $temp_file
    IFS=""

    if [ -z "$cdcatnum" ] ; then
	echo "Sorry, could not extract catalog field from $temp_file"
	get_return
	return 0
    fi 

    echo
    echo " Catalog number  :  $cdcatnum"
    echo "          Title  :  $cdtitle"
    echo "           Type  :  $cdtype"
    echo "Artist/Composer  :  $cdac"
    echo
    get_return

    # 列出CD的曲目信息；
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

# remove，list，update是要操作一张CD，
# 所以执行前，需要先检查是否选择了CD；
# 先删除之前存储的CD曲目信息，然后重新输入；
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
    set $(wc -l $TITLE_FILE)
    num_titles=$1
    set $(wc -l $TRACKS_FILE)
    num_tracks=$1
    echo "found $num_titles CDs, with a total of $num_tracks tracks"
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
