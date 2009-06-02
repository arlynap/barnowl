use strict;
use warnings;

package BarnOwl::Commands;

sub answer_question {
    my $ans = shift;
    my $m = BarnOwl::getcurmsg();
    unless(defined($m)) {
        die("No current message.\n");
    }

    unless($m->is_question) {
        die("That message isn't a question.\n");
    }

    if(defined($m->get_meta('answered'))) {
        die("You already answered that question.\n");
    }

    my $cmd = $m->{$ans."command"};

    if(!defined($cmd)) {
        die("No '$ans' command on this message!\n");
    }

    BarnOwl::command($cmd);

    $m->set_meta(answered => $ans);
    return;
}

sub yes_command {
    answer_question('yes');
}

sub no_command {
    answer_question('no');
}


1;
