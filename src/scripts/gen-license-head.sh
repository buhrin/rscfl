find ../ -name "*.c" -o -name "*.h" -o -name "*.cpp" | xargs -I fname sh -c 'sed -e s/\$\{file}/$(basename ${1})/ -e s/\$\{proj\}/rscfl/ -e s/\$\{proj_url\}/github.com\\/lc525\\/rscfl/ license-head.tpl | echo "$(cat - ${1})" > ${1}' -- fname
