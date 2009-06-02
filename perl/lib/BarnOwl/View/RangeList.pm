use strict;
use warnings;

package BarnOwl::View::RangeList;
use Scalar::Util qw(weaken);
use List::Util qw(max min);

sub next_fwd {shift->{next_fwd}};
sub next_bk  {shift->{next_bk}};

sub next {shift->{next}};
sub prev {shift->{prev}};

sub new {
    my $class = shift;
    my ($bk, $fwd, $prev, $next) = (@_);
    my $self = {
        next_bk  => $bk,
        next_fwd => $fwd,
        next => $next,
        prev => $prev
       };
    bless($self, $class);

    if(defined($self->prev)) {
        weaken($self->{prev});
        $self->prev->{next} = $self;
    }
    if(defined($self->next)) {
        $self->next->{prev} = $self;
    }
    return bless($self, $class);
}

sub expand_fwd {
    my $self = shift;
    my $fwd = shift;
    my $merge = 0;
    $fwd = $self->{next_fwd} + 1 unless defined $fwd;
    $self->{next_fwd} = $fwd;
    while (defined $self->next &&
           $self->next_fwd > $self->next->next_bk) {
        BarnOwl::debug("Merge forward at @{[$self->string]} with @{[$self->next->string]}");
        $merge = 1;
        $self->{next_fwd} = max($self->next_fwd, $self->next->next_fwd);
        $self->{next} = $self->next->next;
        if(defined $self->next) {
            $self->next->{prev} = $self;
            weaken($self->next->{prev});
        }
    }
    return $merge;
}

sub expand_bk {
    my $self = shift;
    my $bk = shift;
    my $merge = 0;
    $bk = $self->{next_bk} - 1 unless defined $bk;
    $self->{next_bk} = $bk;
    while (defined $self->prev &&
           $self->next_bk < $self->prev->next_fwd) {
        BarnOwl::debug("Merge back at @{[$self->string]} with @{[$self->prev->string]}");
        $merge = 1;
        $self->{next_bk} = min($self->next_bk, $self->prev->next_bk);
        $self->{prev} = $self->prev->prev;
        if(defined $self->prev) {
            $self->prev->{next} = $self;
        }
    }
    return $merge;
}

sub find_or_insert {
    my $self = shift;
    my $idx = shift;
    my $range = $self;
    my $last;
    if($idx < 0) {
        while(defined($range->next)) {
            $range = $range->next;
        }
        return BarnOwl::View::RangeList->new($idx, undef, $range, undef);
    }
    while(defined $range) {
        if($idx < $range->next_bk) {
            # We've gone too far, insert a new zero-length range
            BarnOwl::debug("Insert $idx before @{[$range->string]}");
            return BarnOwl::View::RangeList->new($idx, $idx + 1,
                                                 $range->prev,
                                                 $range);
        } elsif($idx >= $range->next_bk &&
                $idx < $range->next_fwd) {
            return $range;
        }
        $last = $range;
        $range = $range->next;
    }

    BarnOwl::debug("Insert at end...");
    return BarnOwl::View::RangeList->new($idx, $idx + 1, $last, undef);
}

sub string {
    my $self = shift;
    my $bk = $self->next_bk;
    $bk = "undef" unless defined $bk;
    my $fd = $self->next_fwd;
    $fd = "undef" unless defined $fd;
    return "[$bk,$fd]";
}

sub string_chain {
    my $range = shift;
    my $str = "";
    while(defined($range)) {
        $str .= $range->string;
        $range = $range->next;
    }
    return $str;
}


1;
