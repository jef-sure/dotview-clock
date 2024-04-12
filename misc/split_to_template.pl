#!/usr/bin/perl
use strict;
use warnings;
use v5.24;
use HTML::Packer;

my $input_file = $ARGV[0];
open( my $fih, "<:encoding(UTF-8)", $input_file )
  or die "cannot open < $input_file: $!";

my $packer = HTML::Packer->init();
my $file_content;
read( $fih, $file_content, -s $fih );
my $mini_content = $packer->minify(
    \$file_content,
    {
        remove_comments => 1,
        remove_newlines => 1,
        do_javascript   => 1
    }
);
my $need_q   = 0;
my $template = <<'EOT';
struct _http_answer {
    const char *const_html;
    str_t *(*handler)();
} template[] = {
EOT

my $handlers = [];
while ( $mini_content =~ /([^%]+)(%\w+%)?/g ) {
    my ( $const, $handler ) = ( $1, $2 );
    $const =~ s/^\s+/ /gm;
    $const  = "@{[$need_q ? '\"':'']}$const";
    $need_q = 0;
    if ( $const =~ /value=$/ ) {
        $need_q = 1;
        $const .= '"';
    }
    if ($handler) {
        $handler = "http_data_" . lc substr( $handler, 1, -1 );
        push @$handlers, $handler;
    }
    else {
        $handler = '0';
    }
    $template .= "{";
    $template .= "\n\t\"$_\"" for map {s/"/\\"/g; split /\r?\n/} unpack "(A130)*", $const;
    $template .= ",//\n\t$handler //\n},";
}

say "static str_t * $_(httpd_req_t *req);" for @$handlers;
say "$template\n};"
