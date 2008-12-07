#!/usr/bin/perl

# Script for turning the RoadMap executable into a cab, along with a shortcut
# Inspired on Eric Houses exe2cab.pl script for the crosswords project.

use strict;

my $version = "-1.2";
my $rmcabname = "wroadmap" . $version;
my $provider = "\"RoadMap project\"";
my $dmcabname .= "demomaps" . $version . ".cab";

sub main() {
    my $path = "wroadmap.exe";

    die "$path not found\n" unless -f $path;

    my $cmdline = "\"%CE1%\\RoadMap\\wroadmap.exe\"";
    my $cmdlen = length( $cmdline );

    my $fname = "/tmp/file$$.list";

    open FILE, "> $fname";

    print FILE 'wroadmap.exe %CE1%\\RoadMap', "\n";

    print FILE '%CE1%\\RoadMap', "\n";

    print FILE 'distribution/libexpat-1.dll %CE1%\\RoadMap', "\n";
    print FILE 'distribution/font.ttf %CE1%\\RoadMap', "\n";
    print FILE 'distribution/roadmap.screenobjects %CE1%\\RoadMap', "\n";
    print FILE 'distribution/roadmap.toolbar %CE1%\\RoadMap', "\n";
    print FILE 'distribution/sprites %CE1%\\RoadMap', "\n";
    print FILE 'distribution/All %CE1%\\RoadMap\\default', "\n";
    print FILE 'distribution/session.txt %CE1%\\RoadMap', "\n";
    print FILE 'distribution/preferences.txt %CE1%\\RoadMap', "\n";

# Create a link.  The format, says Shaun, is
# <number of characters>#command line<no carriage return or line feed>
#   my $linkName = "RoadMap.lnk";
#   open LINKFILE, "> $linkName";
#   print LINKFILE $cmdlen, "#", $cmdline;
#   close LINKFILE;
#
#   print FILE "$linkName ";
#   print FILE '%CE11%', "\n";
#   /* 11 is programs, 14 is games */
#   unlink $linkName;

    close FILE;

    my $appname = $rmcabname;
    $rmcabname .= ".cab";

    my $cmd = "pocketpc-cab -p $provider -a $appname " . "$fname $rmcabname";

    print( STDERR $cmd, "\n");
    print `$cmd`;

#    unlink $tmpfile;
}

sub DemoMapCab() {
    my $fname = "/tmp/file$$.list";

    open FILE, "> $fname";

    print FILE 'demomaps/usc81002.rdm %CE1%\\RoadMap\\Maps', "\n";
    print FILE 'demomaps/usc06075.rdm %CE1%\\RoadMap\\Maps', "\n";
    print FILE 'demomaps/usdir.rdm %CE1%\\RoadMap\\Maps', "\n";

    close FILE;

    my $appname = "\"Demo Maps\"";

    my $cmd = "pocketpc-cab -p $provider -a $appname " . "$fname $dmcabname";
    print( STDERR $cmd, "\n");
    print `$cmd`;

    unlink $fname;
}
main();
DemoMapCab();
