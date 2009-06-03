#!/usr/bin/env perl
use strict;
use warnings;

use Test::More qw(no_plan);
use File::Basename;

BEGIN {
    require (dirname($0) . "/mock.pl");
};

use constant NUM_MESSAGES => 10;

my $ml = BarnOwl::message_list();
my @ms;

for my $i (0..NUM_MESSAGES) {
    my $m = BarnOwl::Message->new(
        type => "generic",
        num    => $i
    );
    push @ms, $m;
    $ml->add_message($m);
}

diag("Testing forward iteration...");
$ml->iterate_begin(0);

for my $i (0..NUM_MESSAGES) {
    my $m = $ml->iterate_next();
    isa_ok($m, "BarnOwl::Message::Generic");
    is($m->{num}, $i);
}

ok(!defined($ml->iterate_next()));
$ml->iterate_done();

diag("Testing reverse iteration...");

$ml->iterate_begin(-1, 1);

for my $i (0..NUM_MESSAGES) {
    my $m = $ml->iterate_next();
    isa_ok($m, "BarnOwl::Message::Generic");
    is($m->{num}, NUM_MESSAGES-$i);
}

ok(!defined($ml->iterate_next()));
$ml->iterate_done();

diag("Test MessageList::binsearch");

my $l = [10..20];
for my $i (10..20) {
    my $idx = BarnOwl::MessageList::binsearch($l, $i);
    is($l->[$idx], $i, "binsearch finds an index that exists");
}

diag("Iterate from a point...");

$ml->iterate_begin($ms[5]->id);
for my $i (5..NUM_MESSAGES) {
    is($ml->iterate_next()->id, $ms[$i]->id);
}
ok(!defined($ml->iterate_next()));
$ml->iterate_done();


diag("Deletion is not immediate...");

$ms[0]->delete;
ok($ms[0]->is_deleted);

$ml->iterate_begin(0);
is($ml->iterate_next()->{num}, 0);
$ml->iterate_done();

diag("Expunge works...");
$ml->expunge();
$ml->iterate_begin(0);
is($ml->iterate_next()->{num}, 1);
$ml->iterate_done();
