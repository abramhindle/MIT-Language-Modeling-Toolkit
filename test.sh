rand ../data/lucene-3.0.3.code.lexed.txt | head -n 10 | \
perl -nle 'my @a = split(/\s+/,$_); print join(" ",@a[0..(@a/2)])' |  \
./estimate-ngram -t ../data/lucene-3.0.3.code.lexed.txt -o 4 -live
#cat test.data | \
