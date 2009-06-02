use strict;
use warnings;

package BarnOwl::View::Iterator;

sub view {return shift->{view}}
sub index {return shift->{index}}

sub new {
    my $class = shift;
    my $self = {
        view     => undef,
        index    => undef,
        at_start => 0,
        at_end   => 0
       };
    return bless $self, $class;
}

sub invalidate {
    my $self = shift;
    $self->{view} = undef;
    $self->{index} = undef;
    $self->{at_start} = $self->{at_end} = 0;
}

sub initialize_at_start {
    my $self = shift;
    my $view = shift;
    $self->{view}  = $view;
    $self->{index} = -1;
    $self->{at_start} = $self->{at_end} = 0;
    $view->recalculate_around(0);
    $self->next;
    BarnOwl::debug("Initialize at start");
}

sub initialize_at_end {
    my $self = shift;
    my $view = shift;
    $self->{view}  = $view;
    $view->recalculate_around(-1);
    $self->{index} = $view->range->next_fwd;
    $self->{at_start} = $self->{at_end} = 0;
    $self->prev;
    BarnOwl::debug("Initialize at end");
}

sub initialize_at_id {
    my $self = shift;
    my $view = shift;
    my $id   = shift;
    $self->{view} = $view;
    $self->{index} = $id;
    $self->{at_start} = $self->{at_end} = 0;
    $view->recalculate_around($id);
    if(!$view->message($id)) {
        $self->next;
    }
    BarnOwl::debug("Initialize at $id");
}

sub clone {
    my $self = shift;
    my $other = shift;
    BarnOwl::debug("clone from @{[$other->{index}||0]}");
    $self->{view} = $other->{view};
    $self->{index} = $other->{index};
    $self->{at_start} = $other->{at_start};
    $self->{at_end} = $other->{at_end};
}

sub has_prev {
    my $self = shift;
    return 0 if $self->at_start;
    my $rv;
    my $idx = $self->index;
    $self->prev;
    $rv = !$self->at_start;
    $self->{index} = $idx;
    $self->{at_start} = 0;
    return $rv;
}

sub has_next {
    my $self = shift;
    return 0 if $self->at_end;
    my $idx = $self->index;
    my $rv;
    $self->next;
    $rv = !$self->at_end;
    $self->{index} = $idx;
    $self->{at_end} = 0;
    return $rv;
}

sub at_start {shift->{at_start}};
sub at_end {shift->{at_end}};

sub valid {
    my $self = shift;
    return defined($self->view) &&
            !$self->at_start &&
            !$self->at_end;
}

sub prev {
    my $self = shift;
    return if $self->at_start;
    $self->{index} = $self->view->range->next_fwd if $self->at_end;
    do {
        $self->{index}--;
        if($self->{index} == $self->view->range->next_bk) {
            $self->view->fill_back;
        }
    } while(!$self->view->message($self->index)
            && $self->index >= 0);

    BarnOwl::debug("PREV newid=@{[$self->index]}");

    if($self->index < 0) {
        BarnOwl::debug("At start");
        $self->{at_start} = 1;
    }
    $self->{at_end} = 0;
}

sub next {
    my $self = shift;
    return if $self->at_end;
    do {
        $self->{index}++;
        if($self->index == $self->view->range->next_fwd) {
            BarnOwl::debug("Forward: fill, id=@{[$self->index]}");
            $self->view->fill_forward;
        }
    } while(!$self->view->message($self->index)
            && $self->index < $self->view->range->next_fwd);

    BarnOwl::debug("NEXT newid=@{[$self->index]}");

    if(!$self->view->message($self->index)) {
        $self->{at_end} = 1;
    }
    $self->{at_start} = 0;
}

sub get_message {
    my $self = shift;
    BarnOwl::debug("get_message: index=@{[$self->index]}");
    return BarnOwl::message_list->get_by_id($self->index);
}

sub cmp {
    my $self = shift;
    my $other = shift;
    return $self->index - $other->index;
}


1;
