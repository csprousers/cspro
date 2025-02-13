package gov.census.cspro.maps;

/**
 * Interface for handling map events (clicks, marker clicks...) see {@link MapEvent}
 */
public interface IMapEventListener
{
    /**
     * Process the event
     * @param e event that occurred on the map
     * @return if event was handled or should be passed on to framework
     */
    boolean onMapEvent(MapEvent e);
}
