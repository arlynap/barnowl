use strict;
use warnings;

package BarnOwl::View;

our %view_cache;

sub message_deleted {
    my $class = shift;
    my $id = shift;
    for my $view (values %view_cache) {
        $view->message($id, 0);
    }
}

sub get_name   {shift->{name}};
sub message {
    my $self = shift;
    my $idx = shift;
    my $val = shift;
    if(defined $val) {
        vec($self->{messages}, $idx, 1) = $val;
    }
    return vec($self->{messages}, $idx, 1);
};
sub get_filter {shift->{filter}};

sub at_start      {shift->{at_start}};
sub at_end        {shift->{at_end}};
sub is_empty      {shift->{is_empty}};
sub range         {shift->{range}};
sub ranges        {shift->{ranges}};

sub new {
    my $class = shift;
    my $name = shift;
    my $filter = shift;
    if(defined($view_cache{$filter})) {
        my $self = $view_cache{$filter};
        return $self;
    }

    my $self  = {messages  => "",
                 name      => $name,
                 filter    => $filter,
                 is_empty  => 1,
                 range     => undef,
                 ranges    => undef};
    bless $self, $class;
    $self->reset;
    $view_cache{$filter} = $self;
    return $self;
}

sub consider_message {
    my $self = shift;
    my $msg = shift;
    for (values %view_cache) {
        $_->_consider_message($msg);
    }
}

sub _consider_message {
    my $self = shift;
    my $msg  = shift;
    return unless $self->at_end;
    if(BarnOwl::filter_message_match($self->get_filter, $msg)) {
        $self->message($msg->{id}, 1);
        $self->{is_empty} = 0;
    }
    $self->range->expand_fwd($msg->{id} + 1);
}

sub reset {
    my $self = shift;
    $self->{messages} = "";
    $self->{at_start} = $self->{at_end} = 0;
    $self->{is_empty} = 1;
    $self->{range} = undef;
    $self->{ranges} = BarnOwl::View::RangeList->new(-1, 0);
}

sub recalculate_around {
    my $self = shift;
    my $where = shift;
    BarnOwl::debug("recalulate @{[$self->get_filter]} around $where");

    if($where == 0) {
        $self->{at_start} = 1;
        $self->{at_end}   = 0;
    } elsif($where < 0) {
        $self->{at_start} = 0;
        $self->{at_end}   = 1;
    } else {
        $self->{at_end} = $self->{at_start} = 0;
    }

    $self->{range} = $self->ranges->find_or_insert($where);
    BarnOwl::debug("[@{[$self->get_filter]}] new range from @{[$self->range->string]}");
    BarnOwl::debug("[@{[$self->get_filter]}]" . $self->ranges->string_chain);

    $self->fill_forward;
    $self->fill_back;
}

my $FILL_STEP = 100;

sub fill_back {
    my $self = shift;
    return if $self->at_start;
    my $pos  = $self->range->next_bk;
    my $ml   = BarnOwl::message_list();
    my $m;

    BarnOwl::debug("Fill back from $pos...");

    $ml->iterate_begin($pos, 1);
    do {
        $m = $ml->iterate_next;

        $self->range->expand_fwd($m->{id} + 1) unless defined $self->range->next_fwd;

        unless(defined $m) {
            BarnOwl::debug("Hit start in fill_back.");
            $self->{at_start} = 1;
            goto loop_done;
        }

        $pos = $m->{id} if $pos < 0;

        if(BarnOwl::filter_message_match($self->get_filter, $m)) {
            $self->{is_empty} = 0;
            $self->message($m->{id}, 1);
        }

        if($self->range->expand_bk($m->{id} - 1)) {
            $ml->iterate_done;
            $ml->iterate_begin($self->range->next_bk, 1);
        }
    } while(($pos - $m->{id}) < $FILL_STEP);

loop_done:
    $ml->iterate_done;
    BarnOwl::debug("[@{[$self->get_filter]}]" . $self->ranges->string_chain);
}

sub fill_forward {
    my $self = shift;
    return if $self->at_end;
    my $pos  = $self->range->next_fwd;
    my $ml   = BarnOwl::message_list();
    my $m;

    BarnOwl::debug("Fill forward from $pos...");

    $ml->iterate_begin($pos, 0);
    do {
        $m = $ml->iterate_next;
        unless(defined $m) {
            BarnOwl::debug("Hit end in fill_forward.");
            $self->{at_end} = 1;
            goto loop_done;
        }

        $pos = $m->{id} if $pos < 0;

        if(BarnOwl::filter_message_match($self->get_filter, $m)) {
            $self->{is_empty} = 0;
            $self->message($m->{id}, 1);
        }
        
        if($self->range->expand_fwd($m->{id} + 1)) {
            $ml->iterate_done;
            $ml->iterate_begin($self->range->next_fwd);
        }
    } while(($m->{id} - $pos) < $FILL_STEP);

loop_done:
    $ml->iterate_done;
    BarnOwl::debug("[@{[$self->get_filter]}]" . $self->ranges->string_chain);
}

sub invalidate_filter {
    my $filter = shift;
    delete $view_cache{$filter};
}


1;
